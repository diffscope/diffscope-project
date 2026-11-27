// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectAudioContext.h"
#include "ProjectAudioContext_p.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>
#include <utility>

#include <QDebug>
#include <QFutureWatcher>
#include <QMutex>
#include <QPromise>
#include <QScopeGuard>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <TalcsCore/AudioBuffer.h>
#include <TalcsCore/AudioDataWrapper.h>
#include <TalcsCore/MixerAudioSource.h>
#include <TalcsCore/PositionableMixerAudioSource.h>
#include <TalcsCore/TransportAudioSource.h>
#include <TalcsDspx/DspxProjectContext.h>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <dspxproject/Document.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <dspxproject/ProjectTimeline.h>
#include <transactional/TransactionController.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <audio/internal/WaveformSingerMetadata.h>

namespace Audio {

    class ProjectDeviceSource : public talcs::AudioSource {
    public:
        explicit ProjectDeviceSource(talcs::MixerAudioSource *mixer) : m_mixer(mixer) {
        }

        bool open(qint64 bufferSize, double sampleRate) override {
            QMutexLocker locker(&m_mutex);
            return (m_reserved || configureMixer(bufferSize, sampleRate)) && AudioSource::open(bufferSize, sampleRate);
        }

        void close() override {
            QMutexLocker locker(&m_mutex);
            if (!m_reserved)
                m_mixer->close();
            AudioSource::close();
        }

        void reserve() {
            QMutexLocker locker(&m_mutex);
            m_reserved = true;
        }

        void setPlaybackSuppressed(bool suppressed) {
            QMutexLocker locker(&m_mutex);
            m_playbackSuppressed = suppressed;
        }

        bool restore(bool exactBufferSize) {
            QMutexLocker locker(&m_mutex);
            bool result = true;
            if (isOpen())
                result = configureMixer(bufferSize(), sampleRate(), exactBufferSize);
            else
                m_mixer->close();
            m_reserved = false;
            return result;
        }

        bool configureMixer(qint64 frames, double rate, bool exactBufferSize = false) const {
            if (m_mixer->isOpen() && m_mixer->sampleRate() == rate &&
                (exactBufferSize ? m_mixer->bufferSize() == frames : m_mixer->bufferSize() >= frames))
                return true;
            m_mixer->close();
            return m_mixer->open(frames, rate);
        }

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &data) override {
            if (m_mutex.tryLock()) {
                const auto unlock = qScopeGuard([this] { m_mutex.unlock(); });
                if (!m_reserved && !m_playbackSuppressed && m_mixer->isOpen())
                    return m_mixer->read(data);
            }
            for (int channel = 0; channel < data.buffer->channelCount(); ++channel)
                data.buffer->clear(channel, data.startPos, data.length);
            return data.length;
        }

    private:
        talcs::MixerAudioSource *m_mixer;
        QMutex m_mutex;
        bool m_reserved = false;
        bool m_playbackSuppressed = false;
    };

    static QFuture<RenderPreparationResult> completedPreparation(RenderPreparationResult result) {
        QPromise<RenderPreparationResult> promise;
        promise.start();
        promise.addResult(std::move(result));
        promise.finish();
        return promise.future();
    }

    class ProjectAudioRenderSessionPrivate {
    public:
        explicit ProjectAudioRenderSessionPrivate(ProjectAudioContext *context) : context(context) {
        }

        ProjectAudioContext *context;
        QMutex mutex;
        ProjectAudioRenderConfig config;
        bool configured = false;
        bool ready = false;
        bool externalPull = false;
        Core::TransactionController::ScopedReadLock readLock;
        talcs::AudioDataWrapper output{nullptr, 0, 0};
        talcs::AudioBuffer stereo;
        qint64 nextPosition = -1;
        QObject observer;
        std::unique_ptr<QPromise<RenderPreparationResult>> preparation;
        QList<QFuture<RenderPreparationResult>> tasks;
        QMetaObject::Connection accessConnection;
        int pending = 0;
        quint64 preparationSerial = 0;

        void complete(RenderPreparationResult result) {
            disconnectAccess();
            ++preparationSerial;
            auto promise = std::move(preparation);
            if (!ready) {
                for (auto &future : tasks)
                    future.cancel();
            }
            tasks.clear();
            pending = 0;
            if (promise) {
                promise->addResult(std::move(result));
                promise->finish();
            }
        }

        void disconnectAccess() {
            QObject::disconnect(accessConnection);
            accessConnection = {};
        }

        void startProviders(const QList<RenderPreparationProvider *> &providers) {
            pending = providers.size();
            const auto serial = preparationSerial;
            if (providers.isEmpty()) {
                seal();
                return;
            }
            for (auto *provider : providers) {
                auto future = provider->prepare(config);
                if (!future.isValid()) {
                    complete({RenderPreparationResult::Failed});
                    return;
                }
                tasks.append(future);
                auto *watcher = new QFutureWatcher<RenderPreparationResult>(&observer);
                QObject::connect(watcher, &QFutureWatcher<RenderPreparationResult>::finished, &observer, [this, watcher, serial] {
                    watcher->deleteLater();
                    if (serial != preparationSerial || !preparation || pending <= 0)
                        return;
                    const auto future = watcher->future();
                    const auto result = future.isCanceled() || future.resultCount() == 0
                        ? RenderPreparationResult{RenderPreparationResult::Cancelled}
                        : future.result();
                    if (result.code != RenderPreparationResult::Ready) {
                        complete(result);
                    } else if (--pending == 0) {
                        tasks.clear();
                        seal();
                    }
                });
                watcher->setFuture(future);
            }
        }

        void seal() {
            if (!preparation)
                return;
            if (preparation->isCanceled()) {
                complete({RenderPreparationResult::Cancelled});
                return;
            }
            auto *controller = context->windowHandle()->projectDocumentContext()->document()->transactionController();
            if (!controller->canAcquireReadLock()) {
                if (!accessConnection) {
                    accessConnection = QObject::connect(controller, &Core::TransactionController::accessStateChanged, &observer, [this] {
                        seal();
                    }, Qt::QueuedConnection);
                }
                return;
            }
            disconnectAccess();
            if (!readLock.acquire(controller)) {
                complete({RenderPreparationResult::Failed});
                return;
            }
            QList<RenderPreparationProvider *> unready;
            for (auto *provider : context->d_func()->preparationProviders.keys()) {
                if (!provider->isReady(config))
                    unready.append(provider);
            }
            if (!unready.isEmpty()) {
                readLock.release();
                startProviders(unready);
                return;
            }
            if (context->d_func()->preparationProviders.isEmpty()) {
                auto *document = context->windowHandle()->projectDocumentContext()->document();
                for (auto *track : document->model()->tracks()->items()) {
                    for (auto *clip : track->clips()->asRange()) {
                        auto *singing = qobject_cast<dspx::SingingClip *>(clip);
                        if (singing && singing->notes()->size() > 0 &&
                            (!singing->sources() || singing->sources()->category() != Internal::WaveformSingerMetadata::architectureId())) {
                            readLock.release();
                            complete({RenderPreparationResult::SourceUnavailable});
                            return;
                        }
                    }
                }
            }
            {
                QMutexLocker locker(&mutex);
                ready = true;
            }
            complete({});
        }

        void readAudio(talcs::AudioSource *source, float *const *channels, qint64 frames) {
            if (config.channels > 1) {
                output.reset(channels, config.channels, frames);
                source->read({&output, 0, frames});
            } else {
                source->read({&stereo, 0, frames});
                const auto *left = stereo.readPointerTo(0, 0);
                const auto *right = stereo.readPointerTo(1, 0);
                for (qint64 frame = 0; frame < frames; ++frame)
                    channels[0][frame] = (left[frame] + right[frame]) * 0.5f;
            }
        }

        static bool rangeReady(const ProjectAudioContextPrivate *context, qint64 position, qint64 frames) {
            for (const auto &provider : context->preparationProviders) {
                if (provider.rangeReady && !provider.rangeReady(position, frames))
                    return false;
            }
            return true;
        }
    };

    ProjectAudioRenderSession::ProjectAudioRenderSession(ProjectAudioContext *context)
        : d_ptr(new ProjectAudioRenderSessionPrivate(context)) {
    }

    ProjectAudioRenderSession::~ProjectAudioRenderSession() {
        release();
    }

    bool ProjectAudioRenderSession::configure(const ProjectAudioRenderConfig &config) {
        auto *d = d_ptr.data();
        QMutexLocker locker(&d->mutex);
        auto *context = d->context;
        if (!context || d->configured || config.maximumFrames <= 0 || !std::isfinite(config.sampleRate) ||
            config.sampleRate <= 0 || config.channels <= 0 || config.channels > ProjectAudioRenderConfig::MaximumChannels ||
            (config.channels > 2 && config.channels % 2 != 0))
            return false;
        auto *contextData = context->d_func();
        if (!static_cast<ProjectDeviceSource *>(contextData->deviceSource.get())->configureMixer(
                config.maximumFrames, config.sampleRate, !d->externalPull))
            return false;
        if (!d->externalPull)
            context->masterTrackMixer()->setRouteChannels(config.channels > 2);
        d->config = config;
        if (config.channels == 1)
            d->stereo.resize(2, config.maximumFrames);
        d->configured = true;
        d->ready = config.mode == ProjectAudioRenderConfig::Mode::Realtime;
        locker.unlock();
        contextData->setPendingSourcePolicy(d->ready ? ProjectAudioContext::PendingSourcePolicy::SkipUnavailable
                                                    : ProjectAudioContext::PendingSourcePolicy::RequireReady);
        Q_EMIT context->sampleRateChanged(config.sampleRate);
        return true;
    }

    QFuture<RenderPreparationResult> ProjectAudioRenderSession::prepare() {
        auto *d = d_ptr.data();
        if (!d->context || !d->configured)
            return completedPreparation({RenderPreparationResult::Failed});
        if (d->ready)
            return completedPreparation({});
        if (d->preparation)
            return d->preparation->future();
        d->preparation = std::make_unique<QPromise<RenderPreparationResult>>();
        d->preparation->start();
        const auto future = d->preparation->future();
        const auto serial = d->preparationSerial;
        auto *watcher = new QFutureWatcher<RenderPreparationResult>(&d->observer);
        QObject::connect(watcher, &QFutureWatcher<RenderPreparationResult>::canceled, &d->observer, [d, serial] {
            if (serial == d->preparationSerial && d->preparation)
                d->complete({RenderPreparationResult::Cancelled});
        });
        QObject::connect(watcher, &QFutureWatcher<RenderPreparationResult>::finished, watcher, &QObject::deleteLater);
        watcher->setFuture(future);
        d->startProviders(d->context->d_func()->preparationProviders.keys());
        return future;
    }

    qint64 ProjectAudioRenderSession::renderBlock(qint64 position, float *const *channels, qint64 frames) {
        return renderBlockInternal(position, channels, frames, false);
    }

    qint64 ProjectAudioRenderSession::renderBlockIfReady(qint64 position, float *const *channels, qint64 frames) {
        return renderBlockInternal(position, channels, frames, true);
    }

    qint64 ProjectAudioRenderSession::renderPlaybackBlockIfReady(float *const *channels, qint64 frames) {
        auto *d = d_ptr.data();
        QMutexLocker locker(&d->mutex);
        if (!d->context || !d->ready || !d->externalPull || d->config.mode != ProjectAudioRenderConfig::Mode::Realtime ||
            frames <= 0 || frames > d->config.maximumFrames)
            return 0;
        auto *contextData = d->context->d_func();
        const std::unique_lock graphLock(contextData->renderGraphMutex, std::try_to_lock);
        if (!graphLock || !contextData->localPlaybackEnabled || contextData->status != ProjectAudioContext::Playing)
            return 0;

        auto *transport = d->context->transport();
        const auto position = transport->position();
        if (position > std::numeric_limits<qint64>::max() - frames)
            return 0;
        const auto [loopStart, loopEnd] = transport->loopingRange();
        if (loopStart < loopEnd && position <= loopEnd && loopEnd - position < frames) {
            const auto first = loopEnd - position;
            if ((first > 0 && !d->rangeReady(contextData, position, first)) ||
                !d->rangeReady(contextData, loopStart, qMin(frames - first, loopEnd - loopStart)))
                return 0;
        } else if (!d->rangeReady(contextData, position, frames)) {
            return 0;
        }
        d->nextPosition = -1;
        d->readAudio(d->context->preMixer(), channels, frames);
        return frames;
    }

    qint64 ProjectAudioRenderSession::renderBlockInternal(qint64 position, float *const *channels, qint64 frames, bool requireReadyRange) {
        auto *d = d_ptr.data();
        QMutexLocker locker(&d->mutex);
        if (!d->context || !d->ready || position < 0 || frames <= 0 || frames > d->config.maximumFrames ||
            frames > std::numeric_limits<qint64>::max() - position)
            return 0;
        auto *contextData = d->context->d_func();
        std::unique_lock graphLock(contextData->renderGraphMutex, std::defer_lock);
        if (requireReadyRange && (!graphLock.try_lock() || !d->rangeReady(contextData, position, frames)))
            return 0;
        auto *mixer = d->context->postMixer();
        if (position != d->nextPosition) {
            contextData->renderSeekGeneration->fetch_add(1, std::memory_order_relaxed);
            mixer->setNextReadPosition(position);
        }
        d->nextPosition = position + frames;
        d->readAudio(mixer, channels, frames);
        return frames;
    }

    bool ProjectAudioRenderSession::release() {
        auto *d = d_ptr.data();
        QMutexLocker locker(&d->mutex);
        auto *context = std::exchange(d->context, nullptr);
        if (!context)
            return true;
        d->ready = false;
        locker.unlock();
        auto *contextData = context->d_func();
        d->complete({RenderPreparationResult::Cancelled});
        d->readLock.release();
        contextData->setPendingSourcePolicy(ProjectAudioContext::PendingSourcePolicy::BufferTransport);
        if (!d->externalPull && d->configured)
            context->masterTrackMixer()->setRouteChannels(context->windowHandle()->projectDocumentContext()->document()->model()->multiChannelOutput());
        auto *deviceSource = static_cast<ProjectDeviceSource *>(contextData->deviceSource.get());
        if (d->externalPull) {
            context->setLocalPlaybackEnabled(false);
            deviceSource->setPlaybackSuppressed(true);
            contextData->setPlaybackStatus(ProjectAudioContext::Stopped);
        }
        const auto restored = deviceSource->restore(!d->externalPull && d->configured);
        Q_EMIT context->sampleRateChanged(context->sampleRate());
        contextData->renderSession = nullptr;
        Q_EMIT context->renderSessionChanged();
        return restored;
    }

    ProjectAudioContext::ProjectAudioContext(Core::ProjectWindowInterface *windowHandle)
        : QObject(windowHandle), d_ptr(new ProjectAudioContextPrivate) {
        Q_D(ProjectAudioContext);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
        d->projectContext = std::make_unique<talcs::DspxProjectContext>(this);
        d->metronomeControlMixer = new talcs::MixerAudioSource;
        d->projectContext->preMixer()->prependSource(d->metronomeControlMixer, true);
        d->deviceSource = std::make_unique<ProjectDeviceSource>(d->projectContext->preMixer());
        windowHandle->addCloseCallback([d] {
            return !d->renderSession || d->renderSession->d_ptr->externalPull;
        });
        connect(windowHandle, &Core::ProjectWindowInterface::loadingStateChanged, this, [d](Core::ExecutiveInterface::State state) {
            if (state == Core::ExecutiveInterface::Exiting)
                d->releaseRenderSession();
        });
        connect(windowHandle, &Core::ProjectWindowInterface::managedCloseRequested, this, [d] {
            if (d->renderSession && d->renderSession->d_ptr->externalPull)
                d->releaseRenderSession();
        });
        connect(windowHandle->projectDocumentContext(), &DspxProject::ProjectDocumentContext::aboutToReload, this, [d] {
            d->releaseRenderSession();
        });
    }

    ProjectAudioContext::~ProjectAudioContext() {
        Q_D(ProjectAudioContext);
        d->releaseRenderSession();
    }

    ProjectAudioContext *ProjectAudioContext::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<ProjectAudioContext>();
    }

    Core::ProjectWindowInterface *ProjectAudioContext::windowHandle() const {
        Q_D(const ProjectAudioContext);
        return d->windowHandle;
    }

    ProjectAudioContext::PlaybackStatus ProjectAudioContext::status() const {
        Q_D(const ProjectAudioContext);
        return d->status;
    }

    void ProjectAudioContext::setStatus(PlaybackStatus status) {
        Q_D(ProjectAudioContext);
        if (!d->usesLocalTransport() || (d->renderSession && (status == Recording || status == RecordingPaused)))
            return;
        switch (status) {
        case Stopped:
        case Playing:
        case Paused:
        case Recording:
        case RecordingPaused:
            break;
        default:
            qWarning() << "The requested project playback status is invalid." << static_cast<int>(status);
            return;
        }
        if (d->playbackStatusChangeHandler && d->playbackStatusChangeHandler(status))
            return;
        if (!d->renderSession && (status == Playing || status == Recording))
            static_cast<ProjectDeviceSource *>(d->deviceSource.get())->setPlaybackSuppressed(false);
        const auto graphLock = lockRenderGraphForUpdate();
        d->setPlaybackStatus(status);
    }

    void ProjectAudioContext::setExternalPlaybackController(QObject *controller) {
        Q_D(ProjectAudioContext);
        d->externalPlaybackController = controller;
    }

    void ProjectAudioContext::setLocalPlaybackEnabled(bool enabled) {
        Q_D(ProjectAudioContext);
        enabled = enabled && d->renderSession && d->renderSession->d_ptr->externalPull &&
            d->renderSession->d_ptr->config.mode == ProjectAudioRenderConfig::Mode::Realtime;
        const auto graphLock = lockRenderGraphForUpdate();
        if (d->localPlaybackEnabled == enabled)
            return;
        d->localPlaybackEnabled = false;
        transport()->pause();
        d->setPlaybackStatus(Stopped);
        d->localPlaybackEnabled = enabled;
    }

    bool ProjectAudioContext::hasRenderSession() const {
        Q_D(const ProjectAudioContext);
        return d->renderSession;
    }

    double ProjectAudioContext::sampleRate() const {
        return preMixer()->sampleRate();
    }

    ProjectAudioContext::PendingSourcePolicy ProjectAudioContext::pendingSourcePolicy() const {
        Q_D(const ProjectAudioContext);
        return d->pendingSourcePolicy;
    }

    std::unique_ptr<ProjectAudioRenderSession> ProjectAudioContext::acquireRenderSession() {
        Q_D(ProjectAudioContext);
        if (d->renderSession || isRecording() || d->windowHandle->isEffectivelyClosed() ||
            !d->windowHandle->projectDocumentContext()->document()->transactionController()->canAcquireReadLock())
            return {};
        static_cast<ProjectDeviceSource *>(d->deviceSource.get())->reserve();
        auto result = std::unique_ptr<ProjectAudioRenderSession>(new ProjectAudioRenderSession(this));
        d->renderSession = result.get();
        Q_EMIT renderSessionChanged();
        return result;
    }

    std::unique_ptr<ProjectAudioRenderSession> ProjectAudioContext::acquireRenderSession(const ProjectAudioRenderConfig &config) {
        auto result = acquireRenderSession();
        if (!result)
            return {};
        result->d_ptr->externalPull = true;
        if (!result->configure(config))
            return {};
        transport()->pause();
        return result;
    }

    void ProjectAudioContext::registerRenderPreparationProvider(RenderPreparationProvider *provider) {
        registerRenderPreparationProvider(provider, {});
    }

    void ProjectAudioContext::registerRenderPreparationProvider(RenderPreparationProvider *provider, std::function<bool(qint64, qint64)> rangeReady) {
        Q_D(ProjectAudioContext);
        auto locker = lockRenderGraphForUpdate();
        if (d->preparationProviders.contains(provider))
            return;
        const auto connection = connect(provider, &QObject::destroyed, this, [this, provider] {
            unregisterRenderPreparationProvider(provider);
        });
        d->preparationProviders.insert(provider, {connection, std::move(rangeReady)});
    }

    void ProjectAudioContext::unregisterRenderPreparationProvider(RenderPreparationProvider *provider) {
        Q_D(ProjectAudioContext);
        auto locker = lockRenderGraphForUpdate();
        const auto iter = d->preparationProviders.find(provider);
        if (iter == d->preparationProviders.end())
            return;
        disconnect(iter->destructionConnection);
        d->preparationProviders.erase(iter);
        locker.unlock();
        if (d->renderSession && d->renderSession->d_ptr->externalPull && d->pendingSourcePolicy == PendingSourcePolicy::RequireReady)
            d->releaseRenderSession();
    }

    QMutexLocker<QRecursiveMutex> ProjectAudioContext::lockRenderGraphForUpdate() {
        Q_D(ProjectAudioContext);
        return QMutexLocker(&d->renderGraphMutex);
    }

    void ProjectAudioContext::setExternalPlaybackState(PlaybackStatus status, qint64 position) {
        Q_D(ProjectAudioContext);
        const double rate = sampleRate();
        if (!d->renderSession || !d->renderSession->d_ptr->externalPull || d->localPlaybackEnabled || rate <= 0 ||
            status == Recording || status == RecordingPaused)
            return;
        auto *timeline = d->windowHandle->projectTimeline();
        const auto milliseconds = static_cast<double>(qMax<qint64>(0, position)) * 1000.0 / rate;
        timeline->setPosition(timeline->musicTimeline()->create(milliseconds).totalTick());
        d->setPlaybackStatus(status);
    }

    bool ProjectAudioContext::isRunning() const {
        const auto currentStatus = status();
        return currentStatus == Playing || currentStatus == Recording;
    }

    bool ProjectAudioContext::isRecording() const {
        const auto currentStatus = status();
        return currentStatus == Recording || currentStatus == RecordingPaused;
    }

    bool ProjectAudioContext::isPaused() const {
        const auto currentStatus = status();
        return currentStatus == Paused || currentStatus == RecordingPaused;
    }

    dspx::SingingClip *ProjectAudioContext::playbackSuppressedClip() const {
        Q_D(const ProjectAudioContext);
        return d->playbackSuppressedClip;
    }

    void ProjectAudioContext::setPlaybackSuppressedClip(dspx::SingingClip *clip) {
        Q_D(ProjectAudioContext);
        if (d->playbackSuppressedClip == clip)
            return;
        disconnect(d->suppressedClipDestroyedConnection);
        d->playbackSuppressedClip = clip;
        if (clip) {
            d->suppressedClipDestroyedConnection = connect(clip, &QObject::destroyed, this, [this] {
                Q_D(ProjectAudioContext);
                d->playbackSuppressedClip.clear();
                Q_EMIT playbackSuppressedClipChanged(nullptr);
            });
        }
        Q_EMIT playbackSuppressedClipChanged(clip);
    }

    talcs::MixerAudioSource *ProjectAudioContext::preMixer() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->preMixer();
    }

    talcs::MixerAudioSource *ProjectAudioContext::metronomeControlMixer() const {
        Q_D(const ProjectAudioContext);
        return d->metronomeControlMixer;
    }

    talcs::TransportAudioSource *ProjectAudioContext::transport() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->transport();
    }

    talcs::PositionableMixerAudioSource *ProjectAudioContext::postMixer() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->postMixer();
    }

    talcs::PositionableMixerAudioSource *ProjectAudioContext::masterControlMixer() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->masterControlMixer();
    }

    talcs::PositionableMixerAudioSource *ProjectAudioContext::masterTrackMixer() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->masterTrackMixer();
    }

    ProjectAudioContext *ProjectAudioContextPrivate::create(Core::ProjectWindowInterface *windowHandle) {
        return new ProjectAudioContext(windowHandle);
    }

    ProjectAudioContextPrivate *ProjectAudioContextPrivate::of(ProjectAudioContext *q) {
        return q->d_func();
    }

    void ProjectAudioContextPrivate::setPendingSourcePolicy(ProjectAudioContext::PendingSourcePolicy policy) {
        if (pendingSourcePolicy == policy)
            return;
        pendingSourcePolicy = policy;
        Q_EMIT q_ptr->pendingSourcePolicyChanged(policy);
    }

    bool ProjectAudioContextPrivate::hasRenderRoutingOverride() const {
        return renderSession && renderSession->d_ptr->configured && !renderSession->d_ptr->externalPull;
    }

    void ProjectAudioContextPrivate::setPlaybackStatus(ProjectAudioContext::PlaybackStatus status) {
        if (this->status == status)
            return;
        this->status = status;
        Q_EMIT q_ptr->statusChanged(status);
        Q_EMIT q_ptr->transportStateChanged();
    }

    bool ProjectAudioContextPrivate::usesLocalTransport() const {
        return renderSession ? localPlaybackEnabled : !externalPlaybackController;
    }

    void ProjectAudioContextPrivate::releaseRenderSession() {
        if (renderSession)
            renderSession->release();
    }

}

#include "moc_ProjectAudioContext.cpp"
