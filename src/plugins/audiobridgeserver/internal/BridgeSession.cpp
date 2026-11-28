// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BridgeSession.h"

#include <bit>
#include <cmath>
#include <limits>
#include <utility>

#include <QDir>
#include <QFutureWatcher>
#include <QScopedValueRollback>
#include <QStringDecoder>
#include <QWindow>

#include <CoreApi/filelocker.h>
#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <dspxproject/Document.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <dspxproject/ProjectLifecycleScenario.h>
#include <dspxproject/ProjectTimeline.h>
#include <transactional/TransactionController.h>

#include <audiobridgeserver/internal/AudioIpc.h>
#include <audiobridgeserver/internal/BridgeServer.h>

namespace AudioBridge::Internal {

    namespace {

        using Playback = Audio::ProjectAudioContext;
        using RenderConfig = Audio::ProjectAudioRenderConfig;
        using Preparation = Audio::RenderPreparationResult;

        static_assert(BridgeMaxAudioChannels <= RenderConfig::MaximumChannels);
        constexpr int timelineUpdateIntervalMs = 33;

        bool zeroBytes(QByteArrayView bytes) {
            for (auto byte : bytes) {
                if (byte != '\0')
                    return false;
            }
            return true;
        }

        ControlErrorCode fileError(DspxProject::ProjectFileError::Code code) {
            using File = DspxProject::ProjectFileError;
            switch (code) {
            case File::None: return ControlErrorCode::None;
            case File::Read: return ControlErrorCode::ReadFailed;
            case File::Parse:
            case File::Load:
            case File::Restore: return ControlErrorCode::LoadFailed;
            case File::Write: return ControlErrorCode::WriteFailed;
            case File::PathInUse: return ControlErrorCode::PathInUse;
            case File::Busy: return ControlErrorCode::Busy;
            case File::Cancelled: return ControlErrorCode::Cancelled;
            case File::TooLarge: return ControlErrorCode::StateTooLarge;
            case File::NeedsConfirmation:
            case File::CheckerWarning:
            case File::IntegrityWarning: return ControlErrorCode::NeedsConfirmation;
            default: return ControlErrorCode::InvalidState;
            }
        }

        QByteArray generationBody(quint32 generation) {
            QByteArray body;
            appendLE(body, generation);
            return body;
        }

        QString filePath(Core::ProjectWindowInterface *window) {
            if (const auto *locker = window->projectDocumentContext()->fileLocker())
                return locker->path();
            return {};
        }

    }

    BridgeSession::BridgeSession(BridgeServer *server, AudioIpc *audio, const QByteArray &instanceId)
        : QObject(server), instanceId(instanceId), m_server(server), m_audioIpc(audio) {
        m_timelineTimer.setInterval(timelineUpdateIntervalMs);
        connect(&m_timelineTimer, &QTimer::timeout, this, &BridgeSession::updateTimeline);
    }

    BridgeSession::~BridgeSession() = default;

    quint64 BridgeSession::connection() const {
        return m_connection;
    }

    bool BridgeSession::canAttach() const {
        return !m_connection && !m_audioDrainWatcher && !m_projectActionPending && !m_persistence;
    }

    bool BridgeSession::initialize(BridgeStateKind kind) {
        m_kind = kind;
        m_window = Core::CoreInterface::createManagedProject(this, kind == BridgeStateKind::ExternalFile, false);
        if (!m_window)
            return false;

        auto *context = m_window->projectDocumentContext();
        connect(context, &DspxProject::ProjectDocumentContext::fileLockerChanged, this, &BridgeSession::watchFilePath);
        connect(context, &DspxProject::ProjectDocumentContext::aboutToReload, this, &BridgeSession::documentReloading);
        connect(context, &DspxProject::ProjectDocumentContext::reloaded, this, &BridgeSession::documentReloaded);
        connect(context->document()->transactionController(), &Core::TransactionController::accessStateChanged,
                this, &BridgeSession::audioSessionChanged, Qt::QueuedConnection);
        if (auto *window = m_window->window())
            connect(window, &QWindow::visibleChanged, this, &BridgeSession::publishStatus);

        m_audio = Playback::of(m_window);
        if (m_audio) {
            m_audio->setExternalPlaybackController(this);
            connect(m_audio, &Playback::renderSessionChanged, this, [this] {
                if (!m_changingRender)
                    QMetaObject::invokeMethod(this, &BridgeSession::audioSessionChanged, Qt::QueuedConnection);
            });
        }

        watchFilePath();
        return true;
    }

    void BridgeSession::attach(quint64 connection, quint32 stateLimit, bool audioCompatible, bool visible, const QByteArray &packet) {
        const QScopedValueRollback attaching(m_attaching, true);
        lease.stop();
        m_connection = connection;
        m_stateLimit = stateLimit;
        m_audioCompatible = audioCompatible;
        if (!packet.isEmpty()) {
            const auto result = restoreState(packet);
            if (result.code != ControlErrorCode::None) {
                m_loadState = ProjectLoadState::LoadFailed;
                m_lastError = result;
            }
        }
        if (m_connection == connection)
            setVisible(visible);
    }

    void BridgeSession::disconnect(int remainingLease) {
        m_connection = 0;
        m_projectDialog.reset();
        m_host = HostTransportState::Unknown;
        setVisible(false);
        lease.start(remainingLease);
        releaseAudioWhenIdle();
    }

    void BridgeSession::close(bool preserveRecoveryLog, bool waitForAudio) {
        m_connection = 0;
        m_closing = true;
        m_projectDialog.reset();
        m_preserveRecoveryLog = preserveRecoveryLog;
        lease.stop();
        setVisible(false);
        if (waitForAudio) {
            delete std::exchange(m_audioDrainWatcher, nullptr);
            releaseAudio();
        } else {
            releaseAudioWhenIdle();
        }
        finishClose();
    }

    void BridgeSession::finishClose() {
        if (!m_closing || m_persistence || m_projectActionPending || m_audioDrainWatcher)
            return;
        if (m_window)
            m_window->closeManagedProject(m_preserveRecoveryLog);
        deleteLater();
    }

    void BridgeSession::dispatch(const ControlRequest &request) {
        if (m_closing)
            return;
        if (m_persistence && request.opcode != ControlOpcode::SetEditorVisible &&
            request.opcode != ControlOpcode::RevealProject && request.opcode != ControlOpcode::EndOffline) {
            m_server->error(request, ControlErrorCode::Busy);
            return;
        }
        if (m_projectActionPending && (request.opcode == ControlOpcode::SaveState || request.opcode == ControlOpcode::RestoreState ||
                                       request.opcode == ControlOpcode::ProjectAction)) {
            m_server->error(request, ControlErrorCode::Busy);
            return;
        }

        switch (request.opcode) {
        case ControlOpcode::ConfigureAudio:
            configureAudio(request);
            break;
        case ControlOpcode::ReleaseAudio:
            releaseAudio();
            m_server->reply(request);
            break;
        case ControlOpcode::Transport:
            transport(request);
            break;
        case ControlOpcode::BeginOffline:
            beginOffline(request);
            break;
        case ControlOpcode::EndOffline:
            endOffline(request);
            break;
        case ControlOpcode::SetEditorVisible: {
            const auto visible = quint8(request.body.at(0));
            if (visible > 1)
                m_server->error(request, ControlErrorCode::InvalidRequest);
            else {
                setVisible(visible);
                m_server->reply(request);
            }
            break;
        }
        case ControlOpcode::RevealProject:
            setVisible(true);
            m_server->reply(request);
            break;
        case ControlOpcode::SaveState:
            saveState(request);
            break;
        case ControlOpcode::ProjectAction:
            projectAction(request);
            break;
        case ControlOpcode::RestoreState: {
            const auto result = restoreState(request.body);
            if (result.code == ControlErrorCode::None)
                m_server->reply(request);
            else
                m_server->error(request, result.code, result.detail);
            break;
        }
        default:
            m_server->error(request, ControlErrorCode::InvalidRequest);
            break;
        }
        publishStatus();
    }

    QByteArray BridgeSession::statusBody() const {
        auto path = m_window && m_kind == BridgeStateKind::ExternalFile ? filePath(m_window).toUtf8() : QByteArray{};
        if (path.size() > std::numeric_limits<quint16>::max())
            path.clear();
        const auto detail = m_lastError.detail.left(20000).toUtf8();
        const auto code = m_lastError.code == ControlErrorCode::None && !m_audioCompatible
            ? ControlErrorCode::Incompatible : m_lastError.code;
        const auto *window = m_window ? m_window->window() : nullptr;
        QByteArray body;
        appendLE(body, m_generation);
        appendLE(body, quint16(code));
        appendLE(body, quint8(m_loadState));
        appendLE(body, quint8(m_audioState));
        appendLE(body, quint8(m_kind));
        appendLE(body, quint8(window && window->isVisible()));
        appendLE(body, quint16(path.size()));
        appendLE(body, quint16(detail.size()));
        appendLE(body, quint16(0));
        body.append(path);
        body.append(detail);
        return body;
    }

    void BridgeSession::publishStatus() {
        if (m_connection && !m_attaching)
            m_server->status(m_connection, statusBody());
    }

    void BridgeSession::setVisible(bool visible) {
        if (auto *window = m_window ? m_window->window() : nullptr) {
            if (visible && window->windowStates().testFlag(Qt::WindowMinimized))
                window->setWindowStates(window->windowStates() & ~Qt::WindowStates(Qt::WindowMinimized));
            window->setVisible(visible);
            if (visible) {
                window->raise();
                window->requestActivate();
            }
        }
    }

    void BridgeSession::watchFilePath() {
        QObject::disconnect(m_filePathConnection);
        if (auto *locker = m_window->projectDocumentContext()->fileLocker())
            m_filePathConnection = connect(locker, &Core::FileLocker::pathChanged, this, &BridgeSession::publishStatus);
        if (!m_reloading) {
            m_kind = m_window->projectDocumentContext()->fileOperationsAllowed()
                ? BridgeStateKind::ExternalFile : BridgeStateKind::HostState;
            if (!m_persistence && !m_projectActionPending)
                publishStatus();
        }
    }

    void BridgeSession::configureAudio(const ControlRequest &request) {
        if (!m_audioCompatible) {
            m_server->error(request, ControlErrorCode::Incompatible);
            return;
        }
        const auto &body = request.body;
        const auto rate = std::bit_cast<double>(readLE<quint64>(body, offsetof(ConfigureAudioRequestV1, sampleRate)));
        const auto frames = readLE<quint32>(body, offsetof(ConfigureAudioRequestV1, maxFrames));
        const auto channels = quint8(body.at(offsetof(ConfigureAudioRequestV1, channels)));
        if (!std::isfinite(rate) || rate <= 0 || frames == 0 || channels == 0 || channels > BridgeMaxAudioChannels ||
            (channels > 2 && channels % 2 != 0) || !zeroBytes(QByteArrayView(body).sliced(offsetof(ConfigureAudioRequestV1, reserved)))) {
            m_server->error(request, ControlErrorCode::InvalidRequest);
            return;
        }
        releaseAudio();
        const auto blockLimit = qMin(frames, maximumBlockFrames);
        m_config = {.maximumFrames = blockLimit, .sampleRate = rate, .channels = channels};
        m_port = m_audioIpc->open(blockLimit, channels);
        if (!m_port) {
            m_lastError = {ControlErrorCode::AudioUnavailable, {}};
            m_server->error(request, m_lastError.code);
            return;
        }
        m_hostPosition = timelinePosition();
        resumeRealtime();

        QByteArray bodyReply;
        appendLE(bodyReply, m_generation);
        appendLE(bodyReply, blockLimit);
        appendLE(bodyReply, quint16(m_port->audioName.size()));
        appendLE(bodyReply, quint16(m_port->wakeName.size()));
        bodyReply.append(m_port->audioName);
        bodyReply.append(m_port->wakeName);
        m_server->reply(request, bodyReply);
    }

    void BridgeSession::transport(const ControlRequest &request) {
        const auto &body = request.body;
        const auto host = static_cast<HostTransportState>(quint8(body.at(offsetof(TransportRequestV1, hostState))));
        const auto command = static_cast<LocalTransportCommand>(quint8(body.at(offsetof(TransportRequestV1, command))));
        const auto hasPosition = quint8(body.at(offsetof(TransportRequestV1, hasPosition)));
        const auto frame = readLE<qint64>(body, offsetof(TransportRequestV1, projectFrame));
        if (host > HostTransportState::Playing || command > LocalTransportCommand::Seek || hasPosition > 1 ||
            (!hasPosition && frame != 0) || (command == LocalTransportCommand::Seek && (!hasPosition || frame < 0)) ||
            !zeroBytes(QByteArrayView(body).sliced(offsetof(TransportRequestV1, reserved)))) {
            m_server->error(request, ControlErrorCode::InvalidRequest);
            return;
        }
        if (m_host != host) {
            const bool wasPlaying = m_host == HostTransportState::Playing;
            m_host = host;
            if (wasPlaying || host == HostTransportState::Playing) {
                if (hasPosition)
                    m_hostPosition = qMax<qint64>(0, frame);
                else if (wasPlaying && m_port)
                    m_hostPosition = m_port->position();
            }
            if (m_audioState == AudioPortState::RealtimeReady) {
                if (wasPlaying && m_audio)
                    m_audio->setExternalPlaybackState(Playback::Stopped, m_hostPosition);
                publishGeneration();
            }
        }
        const auto result = localTransport(command, frame);
        if (result == ControlErrorCode::None)
            m_server->reply(request);
        else
            m_server->error(request, result);
    }

    ControlErrorCode BridgeSession::localTransport(LocalTransportCommand command, qint64 position) {
        if (command == LocalTransportCommand::None)
            return ControlErrorCode::None;
        if (!m_connection)
            return ControlErrorCode::InvalidState;
        if (m_host != HostTransportState::Stopped || m_audioState == AudioPortState::OfflineReady ||
            m_audioState == AudioPortState::OfflinePreparing)
            return ControlErrorCode::TransportOwnedByHost;
        if (!m_port || !m_audio || m_persistence || m_reloading)
            return ControlErrorCode::InvalidState;
        if (!m_render)
            resumeRealtime();
        if (!m_render)
            return ControlErrorCode::AudioUnavailable;

        switch (command) {
        case LocalTransportCommand::Play:
            m_audio->setStatus(Playback::Playing);
            break;
        case LocalTransportCommand::Pause:
            m_audio->setStatus(Playback::Paused);
            break;
        case LocalTransportCommand::Stop:
            m_audio->setStatus(Playback::Stopped);
            break;
        case LocalTransportCommand::Seek: {
            auto *timeline = m_window->projectTimeline();
            const auto milliseconds = static_cast<double>(position) * 1000.0 / m_config.sampleRate;
            timeline->setPosition(timeline->musicTimeline()->create(milliseconds).totalTick());
            break;
        }
        default:
            break;
        }
        return ControlErrorCode::None;
    }

    void BridgeSession::beginOffline(const ControlRequest &request) {
        if (!m_port || m_reloading) {
            m_server->error(request, ControlErrorCode::InvalidState);
            return;
        }
        if (m_audioState == AudioPortState::OfflineReady || m_offlineRequest) {
            m_server->error(request, ControlErrorCode::Busy);
            return;
        }
        releaseRender();
        m_config.mode = RenderConfig::Mode::Offline;
        if (m_audio) {
            const QScopedValueRollback changing(m_changingRender, true);
            m_render = m_audio->acquireRenderSession(m_config);
        }
        if (!m_render) {
            resumeRealtime();
            m_server->error(request, ControlErrorCode::AudioUnavailable);
            return;
        }
        m_lastError = {};
        m_audioState = AudioPortState::OfflinePreparing;
        m_offlineRequest = request;

        auto *watcher = new QFutureWatcher<Preparation>(this);
        m_offlineWatcher = watcher;
        connect(watcher, &QFutureWatcher<Preparation>::finished, this, [this, watcher] {
            if (m_offlineWatcher != watcher || !m_offlineRequest)
                return;
            const auto request = *std::exchange(m_offlineRequest, std::nullopt);
            m_offlineWatcher = nullptr;
            const auto future = watcher->future();
            watcher->deleteLater();
            const auto result = future.isCanceled() || future.resultCount() == 0
                ? Preparation{Preparation::Cancelled} : future.result();
            if (result.code == Preparation::Ready && m_audio && m_audio->hasRenderSession()) {
                m_audioState = AudioPortState::OfflineReady;
                publishGeneration();
                m_server->reply(request, generationBody(m_generation));
            } else {
                releaseRender();
                resumeRealtime();
                const auto code = result.code == Preparation::Cancelled ? ControlErrorCode::Cancelled
                    : result.code == Preparation::SourceUnavailable ? ControlErrorCode::SourceUnavailable
                    : ControlErrorCode::PreparationFailed;
                m_lastError = {code, result.detail};
                m_server->error(request, code, result.detail);
            }
            publishStatus();
        });
        watcher->setFuture(m_render->prepare());
    }

    void BridgeSession::endOffline(const ControlRequest &request) {
        if (m_audioState == AudioPortState::OfflinePreparing || m_audioState == AudioPortState::OfflineReady) {
            releaseRender();
            resumeRealtime();
        }
        m_server->reply(request, generationBody(m_generation));
    }

    void BridgeSession::cancelOffline() {
        if (auto *watcher = std::exchange(m_offlineWatcher, nullptr)) {
            QObject::disconnect(watcher, nullptr, this, nullptr);
            watcher->deleteLater();
        }
        if (const auto request = std::exchange(m_offlineRequest, std::nullopt); request && request->connection == m_connection)
            m_server->error(*request, ControlErrorCode::Cancelled);
    }

    void BridgeSession::releaseRender() {
        if (m_port) {
            m_port->suspend();
            if (m_host == HostTransportState::Playing)
                m_hostPosition = m_port->position();
        }
        m_generation = BridgeNoAudioGeneration;
        m_audioState = m_port ? AudioPortState::Idle : AudioPortState::Unconfigured;
        m_timelineTimer.stop();
        cancelOffline();
        const QScopedValueRollback changing(m_changingRender, true);
        m_render.reset();
    }

    void BridgeSession::releaseAudio() {
        releaseRender();
        if (auto port = std::exchange(m_port, nullptr))
            m_audioIpc->close(port);
        m_audioState = AudioPortState::Unconfigured;
    }

    void BridgeSession::releaseAudioWhenIdle() {
        if (m_audioDrainWatcher)
            return;
        const auto drained = m_port ? m_port->suspendAsynchronously() : std::nullopt;
        if (!drained) {
            releaseAudio();
            return;
        }

        m_generation = BridgeNoAudioGeneration;
        m_audioState = AudioPortState::Idle;
        m_timelineTimer.stop();
        cancelOffline();
        auto *watcher = new QFutureWatcher<void>(this);
        m_audioDrainWatcher = watcher;
        connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher] {
            m_audioDrainWatcher = nullptr;
            watcher->deleteLater();
            releaseAudio();
            finishClose();
        });
        watcher->setFuture(*drained);
    }

    void BridgeSession::resumeRealtime() {
        if (!m_port || !m_audio || m_render || m_persistence || m_reloading || m_closing)
            return;
        m_config.mode = RenderConfig::Mode::Realtime;
        {
            const QScopedValueRollback changing(m_changingRender, true);
            m_render = m_audio->acquireRenderSession(m_config);
        }
        if (!m_render) {
            m_audioState = AudioPortState::Idle;
            m_lastError = {ControlErrorCode::AudioUnavailable, {}};
            return;
        }
        m_audioState = AudioPortState::RealtimeReady;
        m_lastError = {};
        publishGeneration();
    }

    void BridgeSession::publishGeneration() {
        auto mode = AudioPort::Mode::Idle;
        if (m_audioState == AudioPortState::OfflineReady)
            mode = AudioPort::Mode::Offline;
        else if (m_host == HostTransportState::Playing)
            mode = AudioPort::Mode::Host;
        else if (m_host == HostTransportState::Stopped)
            mode = AudioPort::Mode::Preview;

        if (m_audio)
            m_audio->setLocalPlaybackEnabled(mode == AudioPort::Mode::Preview);
        if (++m_nextGeneration == BridgeNoAudioGeneration)
            ++m_nextGeneration;
        m_generation = m_nextGeneration;
        m_port->configure(m_render.get(), mode, m_generation, m_hostPosition);
        if (mode == AudioPort::Mode::Host) {
            updateTimeline();
            m_timelineTimer.start();
        } else {
            m_timelineTimer.stop();
        }
    }

    void BridgeSession::updateTimeline() {
        if (!m_audio || !m_render || !m_port || m_host != HostTransportState::Playing ||
            m_audioState != AudioPortState::RealtimeReady)
            return;
        m_hostPosition = m_port->position();
        m_audio->setExternalPlaybackState(Playback::Playing, m_hostPosition);
    }

    qint64 BridgeSession::timelinePosition() const {
        if (!m_window || m_config.sampleRate <= 0)
            return 0;
        const auto *timeline = m_window->projectTimeline();
        const auto frame = timeline->musicTimeline()->create(0, 0, timeline->position()).millisecond() * m_config.sampleRate / 1000.0;
        if (!std::isfinite(frame) || frame < 0 || frame >= double(std::numeric_limits<qint64>::max()))
            return 0;
        return static_cast<qint64>(frame);
    }

    void BridgeSession::saveState(const ControlRequest &request) {
        if (!m_window || m_reloading) {
            m_server->error(request, ControlErrorCode::Busy);
            return;
        }

        QByteArray data;
        DspxProject::ProjectFileError error;
        {
            const QScopedValueRollback persistence(m_persistence, true);
            auto *scenario = Core::CoreInterface::projectLifecycleScenario();
            auto *context = m_window->projectDocumentContext();
            if (m_kind == BridgeStateKind::ExternalFile) {
                if (filePath(m_window).isEmpty()) {
                    m_lastError = {ControlErrorCode::NeedsProjectPath, {}};
                    m_server->error(request, ControlErrorCode::NeedsProjectPath);
                    return;
                }
                data = filePath(m_window).toUtf8();
                if (data.size() > qsizetype(m_stateLimit) - qsizetype(BridgeStatePacketHeaderBytesV1))
                    error = {DspxProject::ProjectFileError::TooLarge};
                else
                    error = scenario->saveAssociatedFile(context);
            } else {
                error = scenario->saveProjectSnapshot(context, data);
            }
        }
        replyWithState(request, data, error);
    }

    void BridgeSession::projectAction(const ControlRequest &request) {
        using FileReplacement = DspxProject::ProjectLifecycleScenario::FileReplacement;
        using FileError = DspxProject::ProjectFileError;
        const auto action = static_cast<ProjectAction>(quint8(request.body.at(offsetof(ProjectActionRequestV1, action))));
        if (!m_window || m_reloading || m_audioState == AudioPortState::OfflinePreparing || m_audioState == AudioPortState::OfflineReady) {
            m_server->error(request, ControlErrorCode::Busy);
            return;
        }

        QByteArray data;
        FileError error;
        {
            const QScopedValueRollback projectActionPending(m_projectActionPending, true);
            auto *scenario = Core::CoreInterface::projectLifecycleScenario();
            auto *context = m_window->projectDocumentContext();
            const auto payloadLimit = qsizetype(m_stateLimit) - qsizetype(BridgeStatePacketHeaderBytesV1);
            switch (action) {
            case ProjectAction::LinkFile:
            case ProjectAction::ImportToHost: {
                m_projectDialog = std::make_unique<QObject>();
                setVisible(true);
                const auto replacement = action == ProjectAction::LinkFile ? FileReplacement::Associate : FileReplacement::Import;
                if (m_projectDialog)
                    error = scenario->replaceProjectFromFile(context, replacement, data, payloadLimit, m_window->window(), m_projectDialog.get());
                else
                    error = {FileError::Cancelled};
                m_projectDialog.reset();
                if (m_reloading) {
                    m_reloading = false;
                    m_loadState = ProjectLoadState::Ready;
                    resumeRealtime();
                }
                if (error.code == FileError::None && action == ProjectAction::LinkFile)
                    data = filePath(m_window).toUtf8();
                break;
            }
            case ProjectAction::SaveToFile:
                if (m_kind == BridgeStateKind::ExternalFile && !filePath(m_window).isEmpty()) {
                    error = {FileError::InvalidState};
                    break;
                }
                m_projectDialog = std::make_unique<QObject>();
                setVisible(true);
                if (m_projectDialog)
                    error = scenario->saveProjectToFile(context, payloadLimit, m_window->window(), m_projectDialog.get());
                else
                    error = {FileError::Cancelled};
                m_projectDialog.reset();
                if (error.code == FileError::None)
                    data = filePath(m_window).toUtf8();
                break;
            case ProjectAction::StoreInHost:
                if (m_kind != BridgeStateKind::ExternalFile || filePath(m_window).isEmpty()) {
                    error = {FileError::InvalidState};
                    break;
                }
                error = scenario->detachProjectFile(context, data, payloadLimit);
                break;
            default:
                m_server->error(request, ControlErrorCode::InvalidRequest);
                return;
            }
        }
        replyWithState(request, data, error);
    }

    void BridgeSession::replyWithState(const ControlRequest &request, const QByteArray &data, const DspxProject::ProjectFileError &error) {
        if (m_closing) {
            finishClose();
            return;
        }
        if (m_connection != request.connection)
            return;
        m_lastError = {fileError(error.code), error.detail};
        if (m_lastError.code == ControlErrorCode::None && data.size() + BridgeStatePacketHeaderBytesV1 > m_stateLimit)
            m_lastError = {ControlErrorCode::StateTooLarge, {}};
        if (m_lastError.code != ControlErrorCode::None) {
            m_server->error(request, m_lastError.code, m_lastError.detail);
        } else {
            QByteArray packet;
            packet.reserve(BridgeStatePacketHeaderBytesV1 + data.size());
            appendLE(packet, quint16(BridgeStateVersionV1));
            appendLE(packet, quint8(m_kind));
            appendLE(packet, quint8(0));
            packet.append(data);
            m_server->reply(request, packet);
        }
    }

    BridgeSession::StateResult BridgeSession::restoreState(const QByteArray &packet) {
        if (!m_window || m_persistence || m_reloading)
            return {ControlErrorCode::Busy, {}};
        if (packet.size() > m_stateLimit)
            return {ControlErrorCode::StateTooLarge, {}};
        if (packet.size() < BridgeStatePacketHeaderBytesV1 ||
            readLE<quint16>(packet, offsetof(BridgeStatePacketV1, version)) != BridgeStateVersionV1 ||
            packet.at(offsetof(BridgeStatePacketV1, reserved)) != 0)
            return {ControlErrorCode::InvalidState, {}};

        const auto kind = static_cast<BridgeStateKind>(quint8(packet.at(offsetof(BridgeStatePacketV1, kind))));
        const auto data = QByteArrayView(packet).sliced(BridgeStatePacketHeaderBytesV1);
        QString path;
        if (kind == BridgeStateKind::ExternalFile) {
            if (data.isEmpty())
                return {ControlErrorCode::NeedsProjectPath, {}};
            QStringDecoder decoder(QStringDecoder::Utf8);
            path = decoder(data);
            if (decoder.hasError() || path.contains(QChar::Null) || !QDir::isAbsolutePath(path))
                return {ControlErrorCode::InvalidState, {}};
        } else if (kind != BridgeStateKind::HostState || data.isEmpty()) {
            return {ControlErrorCode::InvalidState, {}};
        }

        DspxProject::ProjectFileError error;
        {
            const QScopedValueRollback persistence(m_persistence, true);
            releaseRender();
            m_loadState = ProjectLoadState::Loading;
            publishStatus();
            auto *scenario = Core::CoreInterface::projectLifecycleScenario();
            auto *context = m_window->projectDocumentContext();
            error = kind == BridgeStateKind::ExternalFile ? scenario->restoreProjectFile(context, path)
                                                          : scenario->restoreProjectSnapshot(context, data.toByteArray());
        }
        if (m_closing) {
            finishClose();
            return {ControlErrorCode::Cancelled, {}};
        }
        if (error.code == DspxProject::ProjectFileError::None) {
            m_kind = kind;
            m_loadState = ProjectLoadState::Ready;
            m_hostPosition = timelinePosition();
        } else {
            m_loadState = ProjectLoadState::LoadFailed;
        }
        m_reloading = false;
        resumeRealtime();
        m_lastError = {fileError(error.code), error.detail};
        return m_lastError;
    }

    void BridgeSession::documentReloading() {
        m_reloading = true;
        releaseRender();
        m_loadState = ProjectLoadState::Loading;
        publishStatus();
    }

    void BridgeSession::documentReloaded() {
        m_reloading = false;
        m_loadState = ProjectLoadState::Ready;
        if (m_persistence)
            return;
        m_hostPosition = timelinePosition();
        watchFilePath();
        resumeRealtime();
        publishStatus();
    }

    void BridgeSession::audioSessionChanged() {
        if (m_persistence || m_closing || m_audioDrainWatcher || !m_window)
            return;
        const bool reloadFailed = m_reloading && m_window->projectDocumentContext()->document()->transactionController()->canAcquireReadLock();
        if (reloadFailed) {
            m_reloading = false;
            m_loadState = ProjectLoadState::LoadFailed;
        }
        if (m_port && m_audio && !m_reloading && !m_audio->hasRenderSession()) {
            releaseRender();
            resumeRealtime();
        } else if (!reloadFailed) {
            return;
        }
        if (reloadFailed)
            m_lastError = {ControlErrorCode::LoadFailed, {}};
        publishStatus();
    }

}
