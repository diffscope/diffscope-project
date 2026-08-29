// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisProjectAddOn.h"
#include "SynthesisProjectAddOn_p.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <utility>

#include <QEventLoop>
#include <QLoggingCategory>
#include <QMutex>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftQuick/MessageBox.h>

#include <TalcsCore/MixerAudioSource.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/AudioExporter.h>
#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <dini/engine.h>
#include <dini/transaction.h>
#include <dspxmodelCore/Document.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelPiece/ClipChange.h>
#include <dspxmodelPiece/ClipWatcher.h>
#include <dspxmodelPiece/Piece.h>
#include <dspxmodelPiece/PieceDivider.h>
#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <synth/ProjectSynthesisContext.h>
#include <synth/SynthInterface.h>
#include <synth/SynthesisPiece.h>
#include <synth/SynthesisTask.h>
#include <synth/SynthesisTaskManager.h>
#include <synth/internal/SynthService.h>
#include <synth/internal/SynthesisAudioController.h>
#include <synth/internal/SynthesisDocumentWriter.h>
#include <synth/internal/SynthesisProjectInput.h>
#include <synth/private/ProjectSynthesisContext_p.h>
#include <synth/private/SynthesisPiece_p.h>
#include <transactional/TransactionController.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcSynthesisScheduler, "diffscope.synth.scheduler")

    using namespace ProjectInput;

    class SynthesisExportListener : public Audio::AudioExporterListener {
    public:
        static SynthesisExportListener &instance() {
            static SynthesisExportListener listener;
            return listener;
        }

        void attach(Core::ProjectWindowInterface *window, SynthesisProjectAddOn *addOn) {
            QMutexLocker locker(&m_mutex);
            m_addOns.insert(window, addOn);
        }

        void detach(Core::ProjectWindowInterface *window, SynthesisProjectAddOn *addOn) {
            QMutexLocker locker(&m_mutex);
            if (m_addOns.value(window) == addOn) {
                m_addOns.remove(window);
            }
        }

        bool willStartCallback(Audio::AudioExporter *exporter) override {
            if (!waitForSynthesis(exporter)) {
                return false;
            }
            QString errorMessage;
            if (!refreshAudioRanges(exporter, exporter->config().formatSampleRate(), &errorMessage)) {
                exporter->cancel(
                    true,
                    errorMessage.isEmpty()
                        ? Synth::Internal::SynthesisProjectAddOn::tr("Synthesized audio could not be prepared for the export sample rate")
                        : errorMessage
                );
                return false;
            }
            return true;
        }

        void willFinishCallback(Audio::AudioExporter *exporter) override {
            const auto projectAudioContext = exporter
                                                 ? Audio::ProjectAudioContext::of(exporter->windowHandle())
                                                 : nullptr;
            const auto sampleRate = projectAudioContext
                                        ? projectAudioContext->preMixer()->sampleRate()
                                        : 0.0;
            refreshAudioRanges(exporter, sampleRate);
        }

    private:
        SynthesisExportListener() {
            Audio::AudioExporter::registerListener(this);
        }

        QPointer<SynthesisProjectAddOn> addOnFor(Audio::AudioExporter *exporter) {
            QMutexLocker locker(&m_mutex);
            return m_addOns.value(exporter ? exporter->windowHandle() : nullptr);
        }

        bool refreshAudioRanges(Audio::AudioExporter *exporter, double sampleRate, QString *errorMessage = nullptr) {
            const auto addOn = addOnFor(exporter);
            if (!addOn || qFuzzyIsNull(sampleRate)) {
                return true;
            }
            bool succeeded{};
            const auto refresh = [addOn, sampleRate, errorMessage, &succeeded] {
                if (addOn) {
                    succeeded = addOn->refreshAudioRanges(sampleRate, errorMessage);
                }
            };
            if (QThread::currentThread() == addOn->thread()) {
                refresh();
            } else if (!QMetaObject::invokeMethod(addOn, refresh, Qt::BlockingQueuedConnection)) {
                if (errorMessage) {
                    *errorMessage = Synth::Internal::SynthesisProjectAddOn::tr("Synthesized audio positions could not be updated before export");
                }
                return false;
            }
            return succeeded;
        }

        bool waitForSynthesis(Audio::AudioExporter *exporter) {
            const auto addOn = addOnFor(exporter);
            if (!addOn) {
                return true;
            }
            bool accepted{};
            QString errorMessage;
            const auto wait = [addOn, &accepted, &errorMessage] {
                if (addOn) {
                    accepted = addOn->waitForAudioSynthesis(&errorMessage);
                }
            };
            if (QThread::currentThread() == addOn->thread()) {
                wait();
            } else if (!QMetaObject::invokeMethod(addOn, wait, Qt::BlockingQueuedConnection)) {
                errorMessage = Synth::Internal::SynthesisProjectAddOn::tr("The synthesis state could not be checked before audio export");
                accepted = false;
            }
            if (!accepted && exporter) {
                exporter->cancel(true, errorMessage.isEmpty() ? Synth::Internal::SynthesisProjectAddOn::tr("Audio synthesis did not complete successfully") : errorMessage);
            }
            return accepted;
        }

        QMutex m_mutex;
        QHash<Core::ProjectWindowInterface *, QPointer<SynthesisProjectAddOn>> m_addOns;
    };

    SynthesisProjectAddOn::SynthesisProjectAddOn(QObject *parent)
        : Core::WindowInterfaceAddOn(parent) {
    }

    SynthesisProjectAddOn::~SynthesisProjectAddOn() {
        m_subscription.disconnect();
        auto window = windowHandle() ? windowHandle()->cast<Core::ProjectWindowInterface>() : nullptr;
        SynthesisExportListener::instance().detach(window, this);
        if (m_context) {
            ProjectSynthesisContextPrivate::get(m_context)->controller = nullptr;
        }
        cancelAll();
        const auto clips = m_clips.keys();
        for (auto clip : clips) {
            removeClip(clip);
        }
        qDeleteAll(m_retiredClips);
        qDeleteAll(m_taskWritebacks);
        qDeleteAll(m_pendingManualRequests);
    }

    void SynthesisProjectAddOn::initialize() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        m_taskManager = SynthInterface::instance()->taskManager();
        m_audioController = new SynthesisAudioController(this);
        connect(Audio::GlobalAudioContext::instance(), &Audio::GlobalAudioContext::sampleRateChanged,
                this, [this](double sampleRate) { refreshAudioRanges(sampleRate); });
        connect(m_audioController, &SynthesisAudioController::statusChanged, this, [this](SynthesisPiece *piece) {
            if (!piece) {
                return;
            }
            Q_EMIT m_context->pieceChanged(piece);
            if (!m_audioController->isLoaded(piece)) {
                return;
            }
            queueFinalizer([this, piece = QPointer<SynthesisPiece>(piece)] {
                if (!piece || !m_audioController->isLoaded(piece)) {
                    return;
                }
                auto d = SynthesisPiecePrivate::get(piece);
                d->state = SynthesisPiece::Ready;
                d->errorMessage.clear();
                d->diagnosticFilePath.clear();
                publishPieceState(piece);
                updateCounts();
            });
        });
        m_context = new ProjectSynthesisContext(window, this);
        ProjectSynthesisContextPrivate::get(m_context)->controller = this;
        window->addObject(m_context);
        auto document = window->projectDocumentContext()->document();
        m_transactionController = document->transactionController();
        connect(document->model(), &dspx::Model::globalCentShiftChanged, this, [this] {
            if (!m_internalCommit) {
                m_globalCentShiftPending = true;
            }
        });
        if (m_transactionController) {
            connect(m_transactionController.data(), &Core::TransactionController::transactionActiveChanged, this, [this](bool active) {
                if (!active)
                    schedulePendingWork();
            });
            connect(m_transactionController.data(), &Core::TransactionController::transactionAborted, this, [this] {
                m_documentSyncPending = true;
                m_rollbackPending = true;
                m_globalCentShiftPending = false;
                schedulePendingWork();
            });
        }
        if (auto service = SynthService::instance()) {
            connect(service, &SynthService::managedArchitecturesChanged, this, [this] {
                m_architectureSyncPending = true;
                schedulePendingWork();
            });
        }
        SynthesisExportListener::instance().attach(window, this);

        QQmlComponent actions(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("SynthesisProjectActions"), this);
        if (actions.isError())
            qFatal() << actions.errorString();
        auto actionObject = actions.createWithInitialProperties({
            {QStringLiteral("addOn"), QVariant::fromValue(this)},
        });
        if (!actionObject)
            qFatal() << actions.errorString();
        actionObject->setParent(this);
        QMetaObject::invokeMethod(actionObject, "registerToContext", window->actionContext());

        auto engine = document->model()->document()->engine();
        m_subscription = engine->subscribe([this](const dini::EngineEvent &event) {
            if (event.kind != dini::EventKind::AfterCommit || m_internalCommit) {
                return;
            }
            m_commitPending = true;
            schedulePendingWork();
        });
        connect(window->projectTimeline(), &Core::ProjectTimeline::positionChanged, this, &SynthesisProjectAddOn::updatePriorities);
        if (auto audioContext = Audio::ProjectAudioContext::of(window)) {
            connect(audioContext, &Audio::ProjectAudioContext::statusChanged, this, &SynthesisProjectAddOn::updatePriorities);
        }
        synchronizeDocument();
        QTimer::singleShot(0, this, [this] {
            resynthesizeProject(SynthesisTaskType::Pronunciation, {});
        });
    }

    void SynthesisProjectAddOn::extensionsInitialized() {
    }

    bool SynthesisProjectAddOn::delayedInitialize() {
        return Core::WindowInterfaceAddOn::delayedInitialize();
    }

    ProjectSynthesisContext *SynthesisProjectAddOn::synthesisContext() const {
        return m_context;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::pieces() const {
        QList<SynthesisPiece *> result;
        for (auto runtime : m_clips) {
            if (!runtime || !runtime->clip) {
                continue;
            }
            for (auto piece : runtime->pieces) {
                if (piece && piece->singingClip()) {
                    result.append(piece);
                }
            }
        }
        std::sort(result.begin(), result.end(), [](SynthesisPiece *left, SynthesisPiece *right) {
            const auto leftClip = left ? left->singingClip() : nullptr;
            const auto rightClip = right ? right->singingClip() : nullptr;
            if (!leftClip || !rightClip) {
                return leftClip != nullptr;
            }
            const double leftStart = leftClip->start() + left->position();
            const double rightStart = rightClip->start() + right->position();
            return leftStart < rightStart;
        });
        return result;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::piecesForClip(dspx::SingingClip *clip) const {
        const auto runtime = runtimeForClip(clip);
        if (!runtime || !runtime->divider)
            return {};
        QList<SynthesisPiece *> result;
        for (auto piece : runtime->divider->pieces()) {
            auto synthesisPiece = runtime->pieces.value(piece);
            if (synthesisPiece && synthesisPiece->singingClip())
                result.append(synthesisPiece);
        }
        return result;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::piecesInRange(dspx::SingingClip *clip, double position, double length) const {
        const auto runtime = runtimeForClip(clip);
        if (!runtime || !runtime->divider)
            return {};
        QList<SynthesisPiece *> result;
        for (auto piece : runtime->divider->slice(position, length)) {
            auto synthesisPiece = runtime->pieces.value(piece);
            if (synthesisPiece && synthesisPiece->singingClip())
                result.append(synthesisPiece);
        }
        return result;
    }

    int SynthesisProjectAddOn::synthesizingPieceCount() const {
        return static_cast<int>(std::ranges::count_if(pieces(), [](SynthesisPiece *piece) {
            return piece->state() == SynthesisPiece::Synthesizing;
        }));
    }

    int SynthesisProjectAddOn::queuedPieceCount() const {
        return static_cast<int>(std::ranges::count_if(pieces(), [](SynthesisPiece *piece) {
            return piece->state() == SynthesisPiece::Queued;
        }));
    }

    QList<dspx::SingingClip *> SynthesisProjectAddOn::synchronizeDocument() {
        if (documentTransactionActive()) {
            m_documentSyncPending = true;
            schedulePendingWork();
            return {};
        }
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto model = window->projectDocumentContext()->document()->model();
        QSet<dspx::Handle> live;
        QList<dspx::SingingClip *> added;
        for (auto track : model->tracks()->items()) {
            for (auto clip : track->clips()->asRange()) {
                if (clip->type() == dspx::Clip::Singing) {
                    auto singing = static_cast<dspx::SingingClip *>(clip);
                    if (!isManagedClip(singing))
                        continue;
                    const auto handle = singing->handle();
                    live.insert(handle);
                    if (auto runtime = m_clips.value(handle)) {
                        if (runtime->clip != singing)
                            rebindClip(runtime, singing);
                    } else {
                        addClip(singing);
                        added.append(singing);
                    }
                }
            }
        }
        for (const auto handle : m_clips.keys()) {
            if (!live.contains(handle))
                removeClip(handle);
        }
        return added;
    }

    SynthesisProjectAddOn::ClipRuntime *SynthesisProjectAddOn::runtimeForClip(dspx::SingingClip *clip) const {
        if (!clip)
            return nullptr;
        auto runtime = m_clips.value(clip->handle());
        return runtime && runtime->clip == clip ? runtime : nullptr;
    }

    SynthesisProjectAddOn::ClipRuntime *SynthesisProjectAddOn::runtimeForPiece(SynthesisPiece *piece) const {
        if (!piece)
            return nullptr;
        for (auto runtime : m_clips) {
            if (runtime && runtime->pieces.values().contains(piece))
                return runtime;
        }
        return nullptr;
    }

    void SynthesisProjectAddOn::addClip(dspx::SingingClip *clip) {
        if (!clip || m_clips.contains(clip->handle())) {
            return;
        }
        auto runtime = new ClipRuntime;
        runtime->clipHandle = clip->handle();
        runtime->clip = clip;
        runtime->divider = new dspx::PieceDivider(this);
        runtime->watcher = new dspx::ClipWatcher(this);
        configureDivider(runtime->divider);
        runtime->divider->setSingingClip(clip);
        runtime->watcher->setSingingClip(clip);
        runtime->divider->update();
        runtime->synthesisContextSnapshot = buildSynthesisContext(clip);
        m_clips.insert(runtime->clipHandle, runtime);
        watchClipLifetime(runtime);
        synchronizePieces(runtime);
    }

    void SynthesisProjectAddOn::rebindClip(ClipRuntime *runtime, dspx::SingingClip *clip) {
        if (!runtime || !clip || runtime->clipHandle != clip->handle() || runtime->clip == clip)
            return;
        runtime->clip = clip;
        runtime->rebound = true;
        for (auto piece : runtime->pieces) {
            if (piece)
                SynthesisPiecePrivate::get(piece)->clip = clip;
        }
        configureDivider(runtime->divider);
        runtime->divider->setSingingClip(clip);
        runtime->watcher->setSingingClip(clip);
        runtime->divider->update();
        synchronizePieces(runtime);
        for (auto piece : runtime->pieces) {
            m_audioController->rebindClip(clip, piece);
        }
        watchClipLifetime(runtime);
    }

    void SynthesisProjectAddOn::watchClipLifetime(ClipRuntime *runtime) {
        if (!runtime || !runtime->clip)
            return;
        auto watchedClip = runtime->clip.data();
        connect(watchedClip, &QObject::destroyed, this, [this, runtime, watchedClip] {
            if (m_clips.value(runtime->clipHandle) != runtime)
                return;
            if (runtime->clip && runtime->clip != watchedClip)
                return;
            runtime->clip = nullptr;
            m_documentSyncPending = true;
            schedulePendingWork();
        });
    }

    void SynthesisProjectAddOn::resetClipBaseline(ClipRuntime *runtime) {
        if (!runtime || !runtime->clip || !runtime->divider || !runtime->watcher)
            return;
        runtime->divider->setSingingClip(nullptr);
        runtime->divider->setSingingClip(runtime->clip);
        runtime->divider->update();
        synchronizePieces(runtime);
        runtime->watcher->setSingingClip(nullptr);
        runtime->watcher->setSingingClip(runtime->clip);
        runtime->synthesisContextSnapshot = buildSynthesisContext(runtime->clip);
        runtime->rebound = false;
    }

    void SynthesisProjectAddOn::removeClip(dspx::Handle clipHandle) {
        auto runtime = m_clips.take(clipHandle);
        if (!runtime)
            return;
        runtime->clip = nullptr;
        for (auto piece : runtime->pieces) {
            if (!piece) {
                continue;
            }
            auto d = SynthesisPiecePrivate::get(piece);
            d->clip = nullptr;
            ++d->revision;
            if (auto task = m_pieceTasks.value(piece); task && task->state() == SynthesisTask::Queued) {
                discardTaskWritebacks(task);
                m_taskManager->cancel(task);
            }
            m_pieceTasks.remove(piece);
            removeAudio(piece);
            piece->deleteLater();
        }
        runtime->pieces.clear();
        detachAudioSeries(runtime);
        delete runtime->divider;
        delete runtime->watcher;
        runtime->divider = nullptr;
        runtime->watcher = nullptr;
        m_retiredClips.append(runtime);
        Q_EMIT m_context->piecesChanged();
        updateCounts();
    }

    void SynthesisProjectAddOn::synchronizePieces(ClipRuntime *runtime) {
        if (!runtime || !runtime->clip || !runtime->divider) {
            return;
        }
        const auto sourcePieces = runtime->divider->pieces();
        QSet<dspx::Piece *> live(sourcePieces.cbegin(), sourcePieces.cend());
        bool changed{};
        for (auto source : sourcePieces) {
            auto piece = runtime->pieces.value(source);
            if (!piece) {
                piece = new SynthesisPiece(runtime->clip, m_context);
                runtime->pieces.insert(source, piece);
                connect(piece, &QObject::destroyed, this, [this, piece] {
                    m_pieceTasks.remove(piece);
                    m_audioController->discard(piece);
                });
                changed = true;
            }
            auto d = SynthesisPiecePrivate::get(piece);
            if (d->position != source->position() || d->length != source->length()) {
                d->position = source->position();
                d->length = source->length();
                Q_EMIT piece->rangeChanged();
            }
        }
        for (auto source : runtime->pieces.keys()) {
            if (live.contains(source))
                continue;
            auto piece = runtime->pieces.take(source);
            if (!piece) {
                continue;
            }
            auto d = SynthesisPiecePrivate::get(piece);
            d->clip = nullptr;
            ++d->revision;
            // A language task owns an immutable range and remains useful when a
            // phoneme writeback replaces its SynthesisPiece projection.
            if (auto task = m_pieceTasks.value(piece);
                task && task->state() == SynthesisTask::Queued &&
                task->type() > SynthesisTaskType::Phoneme) {
                discardTaskWritebacks(task);
                m_taskManager->cancel(task);
            }
            m_pieceTasks.remove(piece);
            removeAudio(piece);
            piece->deleteLater();
            changed = true;
        }
        for (auto piece : runtime->pieces) {
            if (piece && !isPieceInSynthesisRange(runtime, piece)) {
                deactivatePiece(piece);
            }
        }
        if (changed)
            Q_EMIT m_context->piecesChanged();
        updateCounts();
    }

    bool SynthesisProjectAddOn::isPieceInSynthesisRange(const ClipRuntime *runtime, const SynthesisPiece *piece) const {
        if (!runtime || !runtime->clip || !piece || piece->singingClip() != runtime->clip) {
            return false;
        }
        const double visibleStart = runtime->clip->clipStart();
        const double visibleEnd = visibleStart + runtime->clip->clipLength();
        return piece->position() + piece->length() > visibleStart &&
               piece->position() < visibleEnd;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::synthesisPiecesForClip(const ClipRuntime *runtime) const {
        if (!runtime || !runtime->clip || !runtime->divider) {
            return {};
        }
        QList<SynthesisPiece *> result;
        for (auto source : runtime->divider->pieces()) {
            auto piece = runtime->pieces.value(source);
            if (isPieceInSynthesisRange(runtime, piece)) {
                result.append(piece);
            }
        }
        return result;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::synthesisPiecesIn(const ClipRuntime *runtime, const QList<SynthesisPiece *> &pieces) const {
        QList<SynthesisPiece *> result;
        result.reserve(pieces.size());
        for (auto piece : pieces) {
            if (isPieceInSynthesisRange(runtime, piece)) {
                result.append(piece);
            }
        }
        return result;
    }

    void SynthesisProjectAddOn::deactivatePiece(SynthesisPiece *piece) {
        if (!piece) {
            return;
        }
        auto d = SynthesisPiecePrivate::get(piece);
        auto task = m_pieceTasks.take(piece);
        const bool changed = d->state != SynthesisPiece::Idle ||
                             !d->errorMessage.isEmpty() ||
                             !d->audioFilePath.isEmpty() ||
                             task;
        if (!changed) {
            return;
        }
        ++d->revision;
        const quint64 revision = d->revision;
        const bool running = task && task->state() == SynthesisTask::Running;
        if (task && task->state() == SynthesisTask::Queued) {
            discardTaskWritebacks(task);
            m_taskManager->cancel(task);
        }
        removeAudio(piece);
        d->errorMessage.clear();
        d->diagnosticFilePath.clear();
        if (running) {
            connect(task, &SynthesisTask::finished, this, [this, piece = QPointer<SynthesisPiece>(piece), revision] {
                queueFinalizer([this, piece, revision] {
                    if (!piece || SynthesisPiecePrivate::get(piece)->revision != revision)
                        return;
                    auto runtime = runtimeForPiece(piece);
                    if (isPieceInSynthesisRange(runtime, piece))
                        return;
                    auto d = SynthesisPiecePrivate::get(piece);
                    d->state = SynthesisPiece::Idle;
                    publishPieceState(piece);
                    updateCounts();
                });
            });
        } else {
            d->state = SynthesisPiece::Idle;
        }
        publishPieceState(piece);
    }

    bool SynthesisProjectAddOn::documentTransactionActive() const {
        if (m_transactionController && m_transactionController->isTransactionActive())
            return true;
        auto window = windowHandle() ? windowHandle()->cast<Core::ProjectWindowInterface>() : nullptr;
        auto document = window && window->projectDocumentContext()
                            ? window->projectDocumentContext()->document()
                            : nullptr;
        auto modelDocument = document && document->model() ? document->model()->document() : nullptr;
        auto transaction = modelDocument ? modelDocument->transaction() : nullptr;
        return transaction && transaction->state() == dini::TransactionState::Active;
    }

    void SynthesisProjectAddOn::schedulePendingWork() {
        if (m_pendingWorkScheduled)
            return;
        m_pendingWorkScheduled = true;
        QMetaObject::invokeMethod(this, [this] {
            m_pendingWorkScheduled = false;
            processPendingWork(); }, Qt::QueuedConnection);
    }

    void SynthesisProjectAddOn::processPendingWork() {
        if (m_processingPendingWork || documentTransactionActive())
            return;
        m_processingPendingWork = true;
        while (!documentTransactionActive()) {
            bool processed{};
            if (m_commitPending) {
                m_commitPending = false;
                m_documentSyncPending = false;
                processCommittedChanges();
                processed = true;
            } else if (m_documentSyncPending) {
                m_documentSyncPending = false;
                synchronizeDocument();
                if (std::exchange(m_rollbackPending, false)) {
                    m_globalCentShiftPending = false;
                    for (auto runtime : m_clips)
                        resetClipBaseline(runtime);
                }
                processed = true;
            }
            if (m_architectureSyncPending) {
                m_architectureSyncPending = false;
                const auto addedClips = synchronizeDocument();
                for (auto clip : addedClips)
                    resynthesizeClip(clip, SynthesisTaskType::Pronunciation, {});
                processed = true;
            }
            if (!m_pendingManualRequests.isEmpty()) {
                const auto requests = std::exchange(m_pendingManualRequests, QList<ManualRequest *>{});
                for (auto request : requests) {
                    processManualRequest(request);
                    delete request;
                }
                processed = true;
            }
            if (!m_pendingFinalizers.isEmpty()) {
                const auto finalizers = std::exchange(m_pendingFinalizers, QList<std::function<void()>>{});
                for (const auto &finalizer : finalizers)
                    finalizer();
                processed = true;
            }
            if (!m_pendingTaskWritebacks.isEmpty()) {
                const auto writebacks = std::exchange(m_pendingTaskWritebacks, QList<TaskWriteback *>{});
                for (auto writeback : writebacks) {
                    auto runtime = writeback ? m_clips.value(writeback->clipHandle) : nullptr;
                    m_taskWritebacks.remove(writeback);
                    processTaskWriteback(writeback);
                    delete writeback;
                    finalizeLanguageWave(runtime);
                }
                processed = true;
            }
            if (!processed)
                break;
        }
        m_processingPendingWork = false;
        if (!documentTransactionActive() &&
            (m_commitPending || m_documentSyncPending || m_architectureSyncPending ||
             !m_pendingManualRequests.isEmpty() || !m_pendingFinalizers.isEmpty() ||
             !m_pendingTaskWritebacks.isEmpty())) {
            schedulePendingWork();
        }
    }

    void SynthesisProjectAddOn::queueFinalizer(std::function<void()> finalizer) {
        m_pendingFinalizers.append(std::move(finalizer));
        schedulePendingWork();
    }

    bool SynthesisProjectAddOn::ensureParameterNodes(ClipRuntime *runtime, const QStringList &parameterIds) {
        if (!runtime || !runtime->clip || parameterIds.isEmpty())
            return true;
        auto model = runtime->clip->model();
        auto document = model ? model->document() : nullptr;
        if (!document || document->transaction())
            return false;
        QStringList missing;
        for (const auto &id : parameterIds) {
            if (!id.isEmpty() && !runtime->clip->parameters()->containsKey(id))
                missing.append(id);
        }
        missing.removeDuplicates();
        if (missing.isEmpty())
            return true;

        auto transaction = document->engine()->beginTransaction({.undoable = false});
        document->setTransaction(&transaction);
        m_internalCommit = true;
        bool succeeded = true;
        try {
            for (const auto &id : missing) {
                auto parameter = model->createParameter();
                if (!runtime->clip->parameters()->insertItem(id, parameter)) {
                    model->destroyItem(parameter);
                    succeeded = false;
                    break;
                }
            }
            if (succeeded)
                transaction.commit();
            else
                transaction.rollback();
        } catch (...) {
            if (transaction.state() == dini::TransactionState::Active)
                transaction.rollback();
            succeeded = false;
        }
        document->setTransaction(nullptr);
        m_internalCommit = false;
        if (!succeeded) {
            qCWarning(lcSynthesisScheduler) << "Failed to create document parameter nodes for synthesis output";
            return false;
        }
        if (runtime->watcher) {
            runtime->watcher->takeChanges();
        }
        return true;
    }

    bool SynthesisProjectAddOn::prepareAudio(ClipRuntime *runtime, SynthesisPiece *piece, QString *errorMessage) {
        if (!runtime) {
            if (errorMessage) {
                *errorMessage = tr("The synthesis piece is no longer available");
            }
            return false;
        }
        return m_audioController->prepare(
            windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip,
            piece, runtime->audioTrackContext, runtime->audioSeries, errorMessage
        );
    }

    void SynthesisProjectAddOn::removeAudio(SynthesisPiece *piece) {
        m_audioController->remove(piece);
    }

    void SynthesisProjectAddOn::detachAudioSeries(ClipRuntime *runtime) {
        if (runtime) {
            m_audioController->detachSeries(runtime->audioTrackContext, runtime->audioSeries);
        }
    }

    bool SynthesisProjectAddOn::installAudio(ClipRuntime *runtime, SynthesisPiece *piece, const QString &filePath, QString *errorMessage) {
        if (!runtime) {
            if (errorMessage) {
                *errorMessage = tr("The synthesis piece is no longer available");
            }
            return false;
        }
        return m_audioController->install(
            windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip,
            piece, runtime->audioTrackContext, runtime->audioSeries, filePath, errorMessage
        );
    }

    bool SynthesisProjectAddOn::refreshAudioRanges(double sampleRate, QString *errorMessage) {
        if (qFuzzyIsNull(sampleRate)) {
            return true;
        }
        bool succeeded = true;
        for (auto runtime : std::as_const(m_clips)) {
            if (!runtime || !runtime->clip) {
                continue;
            }
            const auto pieces = runtime->pieces.values();
            for (auto piece : pieces) {
                if (!piece || !m_audioController->hasBinding(piece)) {
                    continue;
                }
                QString pieceError;
                if (!m_audioController->refreshRange(
                        windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip,
                        piece, runtime->audioTrackContext, runtime->audioSeries,
                        sampleRate, &pieceError)) {
                    if (errorMessage && errorMessage->isEmpty()) {
                        *errorMessage = pieceError;
                    }
                    notifyFailure(piece, pieceError);
                    succeeded = false;
                }
            }
        }
        return succeeded;
    }

    bool SynthesisProjectAddOn::waitForAudioSynthesis(QString *errorMessage) {
        if (documentTransactionActive()) {
            if (errorMessage) {
                *errorMessage = tr("Audio export cannot start while an edit is in progress. Finish or cancel the current edit and try again");
            }
            return false;
        }
        processPendingWork();
        if (documentTransactionActive()) {
            if (errorMessage) {
                *errorMessage = tr("Audio export cannot start while an edit is in progress. Finish or cancel the current edit and try again");
            }
            return false;
        }
        synchronizeDocument();

        QEventLoop eventLoop;
        connect(m_context, &ProjectSynthesisContext::pieceChanged, &eventLoop, &QEventLoop::quit);
        connect(m_context, &ProjectSynthesisContext::piecesChanged, &eventLoop, &QEventLoop::quit);
        connect(m_taskManager, &SynthesisTaskManager::taskChanged, &eventLoop, &QEventLoop::quit);
        if (m_transactionController) {
            connect(m_transactionController.data(), &Core::TransactionController::transactionActiveChanged, &eventLoop, &QEventLoop::quit);
        }

        QSet<dspx::Handle> requestedClips;
        while (true) {
            if (documentTransactionActive()) {
                if (errorMessage) {
                    *errorMessage = tr("Audio export cannot continue while an edit is in progress. Finish or cancel the current edit and try again");
                }
                return false;
            }
            bool waiting{};
            QSet<dspx::Handle> clipsToStart;
            for (auto runtime : m_clips) {
                for (auto piece : synthesisPiecesForClip(runtime)) {
                    if (!piece) {
                        continue;
                    }
                    auto clip = piece->singingClip();
                    if (!clip) {
                        continue;
                    }
                    switch (piece->state()) {
                        case SynthesisPiece::Idle:
                        case SynthesisPiece::Stale:
                            if (requestedClips.contains(runtime->clipHandle)) {
                                if (errorMessage) {
                                    *errorMessage = tr("Audio synthesis could not be started for clip \"%1\"").arg(clip->name());
                                }
                                return false;
                            }
                            clipsToStart.insert(runtime->clipHandle);
                            break;
                        case SynthesisPiece::Queued:
                        case SynthesisPiece::Synthesizing:
                            waiting = true;
                            break;
                        case SynthesisPiece::Ready: {
                            if (!m_audioController->hasBinding(piece) || piece->audioFilePath().isEmpty()) {
                                if (errorMessage) {
                                    *errorMessage = tr("Audio export was stopped because clip \"%1\" contains a synthesis piece without completed audio").arg(clip->name());
                                }
                                return false;
                            }
                            if (m_audioController->isCanceled(piece)) {
                                if (errorMessage) {
                                    *errorMessage = tr("Audio export was stopped because audio synthesis was canceled for a synthesis piece in clip \"%1\"").arg(clip->name());
                                }
                                return false;
                            }
                            if (!m_audioController->isLoaded(piece)) {
                                waiting = true;
                            }
                            break;
                        }
                        case SynthesisPiece::Failed:
                            if (errorMessage) {
                                const auto detail = piece->errorMessage().isEmpty()
                                                        ? tr("Unknown synthesis error")
                                                        : piece->errorMessage();
                                *errorMessage = tr("Audio export was stopped because synthesis failed for a synthesis piece in clip \"%1\": %2")
                                                    .arg(clip->name(), detail);
                            }
                            return false;
                    }
                }
            }

            if (!clipsToStart.isEmpty()) {
                for (const auto clipHandle : clipsToStart) {
                    requestedClips.insert(clipHandle);
                    auto runtime = m_clips.value(clipHandle);
                    if (runtime && runtime->clip)
                        resynthesizeClip(runtime->clip, SynthesisTaskType::Pronunciation, {});
                }
                continue;
            }
            if (!waiting) {
                return true;
            }
            eventLoop.exec();
        }
    }

    void SynthesisProjectAddOn::notifyFailure(SynthesisPiece *piece, const QString &message, const QString &diagnosticFilePath) {
        if (!piece)
            return;
        removeAudio(piece);
        auto d = SynthesisPiecePrivate::get(piece);
        d->state = SynthesisPiece::Failed;
        d->errorMessage = message;
        d->diagnosticFilePath = diagnosticFilePath;
        publishPieceState(piece);
        const auto now = QDateTime::currentDateTimeUtc();
        const auto previous = m_lastNotifications.value(message);
        if (!previous.isValid() || previous.msecsTo(now) > 2000) {
            m_lastNotifications.insert(message, now);
            windowHandle()->cast<Core::ProjectWindowInterface>()->sendNotification(
                SVS::SVSCraft::Critical, tr("Synthesis failed"), message,
                Core::ProjectWindowInterface::AutoHide
            );
        }
        updateCounts();
    }

    using namespace ProjectInput;

    void SynthesisProjectAddOn::invalidate(ClipRuntime *runtime, SynthesisTaskType fromType, const QList<SynthesisPiece *> &affected, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters) {
        const auto synthesisAffected = synthesisPiecesIn(runtime, affected);
        if (synthesisAffected.isEmpty()) {
            updateCounts();
            return;
        }
        QList<SynthesisPiece *> toSchedule;
        for (auto piece : synthesisAffected) {
            if (!piece || !piece->singingClip()) {
                continue;
            }
            auto d = SynthesisPiecePrivate::get(piece);
            d->errorMessage.clear();
            d->diagnosticFilePath.clear();
            if (fromType <= SynthesisTaskType::Audio) {
                removeAudio(piece);
                QString audioError;
                if (!prepareAudio(runtime, piece, &audioError)) {
                    notifyFailure(piece, audioError);
                    continue;
                }
            }
            const auto old = m_pieceTasks.value(piece);
            ClipRuntime::LanguageContinuation *completedLanguageCoverage{};
            ClipRuntime::LanguageContinuation *blockedLanguageCoverage{};
            if (fromType > SynthesisTaskType::Phoneme) {
                for (auto &continuation : runtime->languageContinuations) {
                    if (!rangesOverlap(
                            piece->position(), piece->length(),
                            continuation.position, continuation.length
                        )) {
                        continue;
                    }
                    if (continuation.failed || continuation.canceled) {
                        blockedLanguageCoverage = &continuation;
                    } else {
                        completedLanguageCoverage = &continuation;
                    }
                }
            }
            TaskWriteback *languageCoverage{};
            if (fromType > SynthesisTaskType::Phoneme) {
                for (auto candidate : m_taskWritebacks) {
                    if (!candidate || candidate->discarded || !candidate->task ||
                        candidate->scope != TaskWriteback::Language ||
                        candidate->clipHandle != runtime->clipHandle ||
                        candidate->type >= fromType ||
                        !rangesOverlap(
                            piece->position(), piece->length(),
                            candidate->piecePosition, candidate->pieceLength
                        )) {
                        continue;
                    }
                    if (!languageCoverage || candidate->type < languageCoverage->type ||
                        (candidate->type == languageCoverage->type &&
                         candidate->task->state() == SynthesisTask::Running)) {
                        languageCoverage = candidate;
                    }
                }
            }
            if (blockedLanguageCoverage) {
                if (old && old->state() == SynthesisTask::Queued) {
                    discardTaskWritebacks(old);
                    m_taskManager->cancel(old);
                }
                ++d->revision;
                m_pieceTasks.remove(piece);
                if (!blockedLanguageCoverage->errorMessage.isEmpty()) {
                    notifyFailure(piece, blockedLanguageCoverage->errorMessage);
                    continue;
                }
                removeAudio(piece);
                d->state = SynthesisPiece::Stale;
                d->errorMessage.clear();
                d->diagnosticFilePath.clear();
                publishPieceState(piece);
                continue;
            }
            if (languageCoverage || completedLanguageCoverage) {
                auto coverageTask = languageCoverage ? languageCoverage->task.data() : nullptr;
                if (old && old != coverageTask && old->state() == SynthesisTask::Queued) {
                    discardTaskWritebacks(old);
                    m_taskManager->cancel(old);
                }
                ++d->revision;
                d->state = coverageTask && coverageTask->state() == SynthesisTask::Running
                               ? SynthesisPiece::Synthesizing
                               : SynthesisPiece::Queued;
                d->currentTaskType = languageCoverage
                                         ? languageCoverage->type
                                         : nextStage(completedLanguageCoverage->architecture, SynthesisTaskType::Phoneme);
                if (coverageTask) {
                    m_pieceTasks.insert(piece, coverageTask);
                } else {
                    m_pieceTasks.remove(piece);
                }
                publishPieceState(piece);
                continue;
            }
            const bool activeUpstreamTask = old && hasUnprocessedWriteback(old) &&
                                            old->type() < fromType;
            const bool coveredByCurrentPipeline = activeUpstreamTask;
            if (!coveredByCurrentPipeline) {
                ++d->revision;
                d->state = SynthesisPiece::Stale;
                if (old && old->state() == SynthesisTask::Queued) {
                    m_taskManager->cancel(old);
                }
                toSchedule.append(piece);
                Q_EMIT piece->stateChanged();
            }
            Q_EMIT m_context->pieceChanged(piece);
        }
        if (fromType <= SynthesisTaskType::Phoneme) {
            for (auto piece : toSchedule)
                schedulePieceLanguage(runtime, piece, fromType, options);
        } else {
            for (auto piece : toSchedule)
                schedulePieceStage(runtime, piece, fromType, options, requestedParameters);
        }
        updateCounts();
    }

    void SynthesisProjectAddOn::schedulePieceLanguage(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!runtime || !runtime->clip || !runtime->divider || !piece || piece->singingClip() != runtime->clip)
            return;
        if (!isPieceInSynthesisRange(runtime, piece)) {
            deactivatePiece(piece);
            return;
        }
        scheduleLanguageRange(runtime, piece->position(), piece->length(), fromType, options);
    }

    void SynthesisProjectAddOn::scheduleLanguageRange(ClipRuntime *runtime, double position, double length, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!runtime || !runtime->clip || !runtime->divider || length <= 0.0 || documentTransactionActive())
            return;
        if (!isManagedClip(runtime->clip)) {
            removeClip(runtime->clipHandle);
            return;
        }
        const auto affectedPieces = synthesisPiecesIn(
            runtime, piecesInRange(runtime->clip, position, length)
        );
        if (affectedPieces.isEmpty())
            return;
        const auto context = buildSynthesisContext(runtime->clip);
        if (!context) {
            for (auto piece : affectedPieces)
                notifyFailure(piece, tr("The clip has no valid singer source"));
            return;
        }
        const auto architecture = architectureFor(*context);
        if (architecture.id().isEmpty()) {
            for (auto piece : affectedPieces)
                notifyFailure(piece, tr("No healthy service supports the architecture used by this clip"));
            return;
        }
        QList<ClipRuntime::LanguageContinuation> preservedContinuations;
        for (const auto &continuation : runtime->languageContinuations) {
            if (!rangesOverlap(position, length, continuation.position, continuation.length))
                continue;
            for (auto piece : synthesisPiecesIn(runtime, piecesInRange(runtime->clip, continuation.position, continuation.length))) {
                if (rangesOverlap(position, length, piece->position(), piece->length()))
                    continue;
                auto preserved = continuation;
                preserved.position = piece->position();
                preserved.length = piece->length();
                preservedContinuations.append(preserved);
            }
        }
        runtime->languageContinuations.erase(
            std::remove_if(
                runtime->languageContinuations.begin(), runtime->languageContinuations.end(),
                [position, length](const ClipRuntime::LanguageContinuation &continuation) {
                    return rangesOverlap(position, length, continuation.position, continuation.length);
                }
            ),
            runtime->languageContinuations.end()
        );
        runtime->languageContinuations.append(preservedContinuations);
        QHash<SynthesisPiece *, TaskWriteback *> deferredLanguagePieces;
        for (auto oldWriteback : m_taskWritebacks) {
            if (!oldWriteback || oldWriteback->discarded ||
                oldWriteback->scope != TaskWriteback::Language ||
                oldWriteback->clipHandle != runtime->clipHandle ||
                !rangesOverlap(position, length, oldWriteback->piecePosition, oldWriteback->pieceLength)) {
                continue;
            }
            for (auto piece : synthesisPiecesIn(runtime, piecesInRange(runtime->clip, oldWriteback->piecePosition, oldWriteback->pieceLength))) {
                if (rangesOverlap(position, length, piece->position(), piece->length()))
                    continue;
                const auto existing = deferredLanguagePieces.value(piece);
                if (!existing || oldWriteback->type < existing->type)
                    deferredLanguagePieces.insert(piece, oldWriteback);
            }
            oldWriteback->discarded = true;
            if (oldWriteback->task && oldWriteback->task->state() == SynthesisTask::Queued)
                m_taskManager->cancel(oldWriteback->task);
        }
        for (auto it = deferredLanguagePieces.cbegin(); it != deferredLanguagePieces.cend(); ++it) {
            auto piece = it.key();
            auto oldWriteback = it.value();
            if (!piece || !oldWriteback)
                continue;
            schedulePieceLanguage(
                runtime, piece, oldWriteback->type, oldWriteback->options
            );
        }
        fromType = executableStage(architecture, fromType);
        if (fromType == SynthesisTaskType::Parameter) {
            for (auto piece : affectedPieces)
                schedulePieceStage(runtime, piece, SynthesisTaskType::Parameter, options);
            return;
        }
        const auto built = buildLanguageRequest(runtime->clip, position, length, fromType, *context);
        if (built.noteHandles.isEmpty()) {
            for (auto piece : affectedPieces)
                notifyFailure(piece, tr("The synthesis task could not be queued"));
            return;
        }
        for (auto piece : affectedPieces) {
            const auto oldTask = m_pieceTasks.value(piece);
            if (oldTask && oldTask->state() == SynthesisTask::Queued) {
                discardTaskWritebacks(oldTask);
                m_taskManager->cancel(oldTask);
            }
            if (!m_audioController->hasBinding(piece)) {
                QString audioError;
                if (!prepareAudio(runtime, piece, &audioError)) {
                    notifyFailure(piece, audioError);
                    return;
                }
            }
        }
        auto task = m_taskManager->enqueue(built.request, options);
        if (!task) {
            for (auto piece : affectedPieces)
                notifyFailure(piece, tr("The synthesis task could not be queued"));
            return;
        }
        for (auto piece : affectedPieces) {
            auto d = SynthesisPiecePrivate::get(piece);
            ++d->revision;
            d->state = SynthesisPiece::Queued;
            d->currentTaskType = fromType;
            d->errorMessage.clear();
            d->diagnosticFilePath.clear();
            m_pieceTasks.insert(piece, task);
            publishPieceState(piece);
        }
        auto writeback = new TaskWriteback;
        writeback->scope = TaskWriteback::Language;
        writeback->task = task;
        writeback->clipHandle = runtime->clipHandle;
        writeback->type = fromType;
        writeback->options = options;
        writeback->architecture = architecture;
        writeback->request = built.request;
        writeback->noteHandles = built.noteHandles;
        writeback->piecePosition = position;
        writeback->pieceLength = length;
        m_taskWritebacks.insert(writeback);
        connect(task, &SynthesisTask::stateChanged, this, [this, clipHandle = runtime->clipHandle, task, fromType, position, length] {
            if (task->state() != SynthesisTask::Running)
                return;
            queueFinalizer([this, clipHandle, task, fromType, position, length] {
                auto runtime = m_clips.value(clipHandle);
                if (!runtime || !runtime->clip || task->state() != SynthesisTask::Running ||
                    !hasUnprocessedWriteback(task)) {
                    return;
                }
                for (auto currentPiece : synthesisPiecesIn(
                         runtime, piecesInRange(runtime->clip, position, length)
                     )) {
                    auto d = SynthesisPiecePrivate::get(currentPiece);
                    d->state = SynthesisPiece::Synthesizing;
                    d->currentTaskType = fromType;
                    m_pieceTasks.insert(currentPiece, task);
                    publishPieceState(currentPiece);
                }
                updateCounts();
            });
        });
        connect(task, &SynthesisTask::finished, this, [this, writeback] {
            queueTaskWriteback(writeback);
        });
        if (task->isFinished())
            queueTaskWriteback(writeback);
        updatePriorities();
        updateCounts();
    }

    void SynthesisProjectAddOn::schedulePieceStage(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTaskType type, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters) {
        if (!runtime || !runtime->clip || !piece || piece->singingClip() != runtime->clip)
            return;
        if (documentTransactionActive())
            return;
        if (!isManagedClip(runtime->clip)) {
            removeClip(runtime->clipHandle);
            return;
        }
        if (!isPieceInSynthesisRange(runtime, piece)) {
            deactivatePiece(piece);
            return;
        }
        QString audioError;
        if (!prepareAudio(runtime, piece, &audioError)) {
            notifyFailure(piece, audioError);
            return;
        }
        const auto context = buildSynthesisContext(runtime->clip);
        const auto architecture = context ? architectureFor(*context) : ArchitectureMetadata{};
        if (!context || architecture.id().isEmpty()) {
            notifyFailure(piece, tr("No healthy service can synthesize this synthesis piece"));
            return;
        }
        const auto executableType = executableStage(architecture, type);
        if (executableType != type) {
            schedulePieceStage(runtime, piece, executableType, options, requestedParameters);
            return;
        }
        const bool forAudio = type == SynthesisTaskType::Audio;
        auto built = buildScore(windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip, piece, architecture, forAudio, requestedParameters);
        if (!built.error.isEmpty()) {
            notifyFailure(piece, built.error);
            return;
        }
        if (type == SynthesisTaskType::Parameter && built.score.requestedParameters.isEmpty()) {
            schedulePieceStage(runtime, piece, SynthesisTaskType::Audio, options);
            return;
        }
        SynthesisTaskRequest request;
        request.type = type;
        request.context = *context;
        request.score = built.score;
        request.displayName = runtime->clip->name();
        const quint64 revision = SynthesisPiecePrivate::get(piece)->revision;
        auto task = m_taskManager->enqueue(request, options);
        if (!task) {
            notifyFailure(piece, tr("The synthesis task could not be queued"));
            return;
        }
        m_pieceTasks.insert(piece, task);
        bindPieceTask(runtime, piece, task, revision, options);
        auto writeback = new TaskWriteback;
        writeback->scope = TaskWriteback::Piece;
        writeback->task = task;
        writeback->clipHandle = runtime->clipHandle;
        writeback->piece = piece;
        writeback->type = type;
        writeback->options = options;
        writeback->architecture = architecture;
        writeback->request = request;
        writeback->noteHandles = built.noteHandles;
        writeback->requestedParameters = requestedParameters;
        writeback->revision = revision;
        writeback->piecePosition = piece->position();
        writeback->pieceLength = piece->length();
        m_taskWritebacks.insert(writeback);
        connect(task, &SynthesisTask::finished, this, [this, writeback] {
            queueTaskWriteback(writeback);
        });
        if (task->isFinished())
            queueTaskWriteback(writeback);
        updatePriorities();
    }

    void SynthesisProjectAddOn::bindPieceTask(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTask *task, quint64 revision, const SynthesisTaskOptions &) {
        auto d = SynthesisPiecePrivate::get(piece);
        d->state = SynthesisPiece::Queued;
        d->currentTaskType = task->type();
        d->errorMessage.clear();
        d->diagnosticFilePath.clear();
        publishPieceState(piece);
        connect(task, &SynthesisTask::stateChanged, this, [this, runtime, piece = QPointer<SynthesisPiece>(piece), task, revision] {
            if (task->state() != SynthesisTask::Running)
                return;
            queueFinalizer([this, runtime, piece, task, revision] {
                if (m_clips.value(runtime->clipHandle) != runtime || !piece ||
                    SynthesisPiecePrivate::get(piece)->revision != revision ||
                    task->state() != SynthesisTask::Running) {
                    return;
                }
                auto d = SynthesisPiecePrivate::get(piece);
                d->state = SynthesisPiece::Synthesizing;
                publishPieceState(piece);
                updateCounts();
            });
        });
        updateCounts();
    }

    void SynthesisProjectAddOn::resynthesizeProject(SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (documentTransactionActive()) {
            auto request = new ManualRequest;
            request->scope = ManualRequest::Project;
            request->fromType = fromType;
            request->options = options;
            m_pendingManualRequests.append(request);
            schedulePendingWork();
            return;
        }
        synchronizeDocument();
        for (auto runtime : m_clips) {
            if (!runtime || !runtime->clip || !runtime->divider) {
                continue;
            }
            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
            invalidate(runtime, fromType, synthesisPiecesForClip(runtime), options);
        }
        updatePriorities();
    }

    void SynthesisProjectAddOn::resynthesizeClip(dspx::SingingClip *clip, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!clip)
            return;
        if (documentTransactionActive()) {
            auto request = new ManualRequest;
            request->scope = ManualRequest::Clip;
            request->clipHandle = clip->handle();
            request->fromType = fromType;
            request->options = options;
            m_pendingManualRequests.append(request);
            schedulePendingWork();
            return;
        }
        if (clip && !isManagedClip(clip)) {
            removeClip(clip->handle());
            return;
        }
        if (auto runtime = runtimeForClip(clip); runtime && runtime->divider) {
            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
            invalidate(runtime, fromType, synthesisPiecesForClip(runtime), options);
        }
        updatePriorities();
    }

    void SynthesisProjectAddOn::resynthesizePiece(SynthesisPiece *piece, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!piece)
            return;
        auto runtime = runtimeForPiece(piece);
        auto clip = piece->singingClip();
        if (documentTransactionActive()) {
            if (!runtime && !clip)
                return;
            auto request = new ManualRequest;
            request->scope = ManualRequest::Piece;
            request->clipHandle = runtime ? runtime->clipHandle : clip->handle();
            request->position = piece->position();
            request->length = piece->length();
            request->fromType = fromType;
            request->options = options;
            m_pendingManualRequests.append(request);
            schedulePendingWork();
            return;
        }
        if (!clip)
            return;
        if (!isManagedClip(clip)) {
            removeClip(clip->handle());
            return;
        }
        runtime = runtimeForClip(clip);
        if (!runtime || !runtime->clip || !runtime->divider)
            return;
        configureDivider(runtime->divider);
        runtime->divider->update();
        synchronizePieces(runtime);
        if (!runtime->pieces.values().contains(piece))
            return;
        if (!isPieceInSynthesisRange(runtime, piece))
            return;
        invalidate(runtime, fromType, {piece}, options);
        updatePriorities();
    }

    void SynthesisProjectAddOn::resynthesizeSelectedItems() {
        auto *windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto *window = qobject_cast<QQuickWindow *>(windowInterface ? windowInterface->window() : nullptr);
        auto *document = windowInterface ? windowInterface->projectDocumentContext()->document() : nullptr;
        auto *selectionModel = document ? document->selectionModel() : nullptr;
        if (!window || !selectionModel)
            return;
        const auto selectionType = selectionModel->selectionType();
        if (selectionType != dspx::SelectionModel::ST_Clip && selectionType != dspx::SelectionModel::ST_Note)
            return;

        QList<dspx::SingingClip *> clips;
        QList<dspx::Note *> notes;
        dspx::NoteSequence *noteSequence{};
        if (selectionType == dspx::SelectionModel::ST_Clip) {
            const auto selectedClips = selectionModel->clipSelectionModel()->selectedItems();
            clips.reserve(selectedClips.size());
            for (auto *item : selectedClips) {
                if (auto *singingClip = qobject_cast<dspx::SingingClip *>(item)) {
                    clips.append(singingClip);
                }
            }
        } else {
            auto *noteSelectionModel = selectionModel->noteSelectionModel();
            noteSequence = noteSelectionModel->noteSequenceWithSelectedItems();
            if (!noteSequence)
                return;
            if (auto *singingClip = noteSequence->singingClip()) {
                clips.append(singingClip);
            }
            notes = noteSelectionModel->selectedItems();
            if (notes.isEmpty())
                return;
        }
        if (clips.isEmpty())
            return;

        QStringList unmanagedNames;
        for (auto *clip : clips) {
            if (!isManagedClip(clip)) {
                unmanagedNames.append(clip->name());
            }
        }
        if (!unmanagedNames.isEmpty()) {
            SVS::MessageBox::warning(
                Core::RuntimeInterface::qmlEngine(),
                window,
                tr("Resynthesize"),
                tr("The following clips cannot be resynthesized because their sources are not managed by a synthesis service:\n%1").arg(unmanagedNames.join(", "))
            );
            return;
        }

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("ResynthesizeDialog"));
        if (component.isError()) {
            qWarning() << component.errorString();
            return;
        }
        std::unique_ptr<QObject> dialog(component.createWithInitialProperties({
            {QStringLiteral("parent"), QVariant::fromValue(window->contentItem())},
            {QStringLiteral("resynthesizeFrom"), 0},
            {QStringLiteral("disableCache"), false},
        }));
        if (!dialog) {
            qWarning() << component.errorString();
            return;
        }
        dialog->setProperty("x", window->width() / 2.0 - dialog->property("width").toDouble() / 2.0);
        if (const auto topMargin = window->property("popupTopMarginHint"); topMargin.isValid()) {
            dialog->setProperty("y", topMargin);
        } else {
            dialog->setProperty("y", window->height() / 2.0 - dialog->property("height").toDouble() / 2.0);
        }
        QEventLoop eventLoop;
        QObject::connect(dialog.get(), SIGNAL(accepted()), &eventLoop, SLOT(quit()));
        QObject::connect(dialog.get(), SIGNAL(rejected()), &eventLoop, SLOT(quit()));
        QMetaObject::invokeMethod(dialog.get(), "open");
        eventLoop.exec();
        if (dialog->property("result").toInt() != 1)
            return;

        const auto fromType = static_cast<SynthesisTaskType>(std::clamp(dialog->property("resynthesizeFrom").toInt(), 0, static_cast<int>(SynthesisTaskType::Audio)));
        const SynthesisTaskOptions options{!dialog->property("disableCache").toBool(), true, 0};

        if (selectionType == dspx::SelectionModel::ST_Clip) {
            for (auto *clip : clips) {
                resynthesizeClip(clip, fromType, options);
            }
            return;
        }

        std::sort(notes.begin(), notes.end(), [](const dspx::Note *lhs, const dspx::Note *rhs) {
            return lhs->position() < rhs->position();
        });
        QSet<SynthesisPiece *> pieces;
        const auto collect = [&](double position, double length) {
            const auto found = piecesInRange(noteSequence->singingClip(), position, length);
            for (auto *piece : found) {
                pieces.insert(piece);
            }
        };
        int intervalStart = -1;
        int intervalEnd = -1;
        for (const auto *note : std::as_const(notes)) {
            const int start = note->position();
            const int end = note->position() + note->length();
            if (intervalStart < 0 || start > intervalEnd) {
                if (intervalStart >= 0) {
                    collect(intervalStart, intervalEnd - intervalStart);
                }
                intervalStart = start;
                intervalEnd = end;
            } else if (end > intervalEnd) {
                intervalEnd = end;
            }
        }
        if (intervalStart >= 0) {
            collect(intervalStart, intervalEnd - intervalStart);
        }
        for (auto *piece : pieces) {
            resynthesizePiece(piece, fromType, options);
        }
    }

    bool SynthesisProjectAddOn::cancelPiece(SynthesisPiece *piece) {
        if (!piece)
            return false;
        QSet<SynthesisTask *> tasks;
        bool canceledContinuation{};
        if (auto task = m_pieceTasks.value(piece);
            task && (!task->isFinished() || hasUnprocessedWriteback(task))) {
            tasks.insert(task);
        }
        const auto runtime = runtimeForPiece(piece);
        if (runtime) {
            for (auto &continuation : runtime->languageContinuations) {
                if (!rangesOverlap(
                        piece->position(), piece->length(),
                        continuation.position, continuation.length
                    )) {
                    continue;
                }
                continuation.canceled = true;
                continuation.errorMessage.clear();
                canceledContinuation = true;
            }
            for (auto writeback : m_taskWritebacks) {
                if (!writeback || writeback->discarded || !writeback->task ||
                    writeback->scope != TaskWriteback::Language ||
                    writeback->clipHandle != runtime->clipHandle ||
                    !rangesOverlap(piece->position(), piece->length(), writeback->piecePosition, writeback->pieceLength)) {
                    continue;
                }
                runtime->languageContinuations.append({
                    writeback->piecePosition,
                    writeback->pieceLength,
                    writeback->options,
                    writeback->architecture,
                    true,
                    false,
                    {},
                });
                writeback->discarded = true;
                tasks.insert(writeback->task);
            }
        }
        if (tasks.isEmpty() && !canceledContinuation)
            return false;
        for (auto writeback : m_taskWritebacks) {
            if (writeback && tasks.contains(writeback->task))
                writeback->discarded = true;
        }
        QSet<SynthesisPiece *> affectedPieces{piece};
        for (auto writeback : m_taskWritebacks) {
            if (!writeback || writeback->scope != TaskWriteback::Language ||
                !tasks.contains(writeback->task)) {
                continue;
            }
            auto taskRuntime = m_clips.value(writeback->clipHandle);
            if (!taskRuntime || !taskRuntime->clip)
                continue;
            const auto current = synthesisPiecesIn(taskRuntime, piecesInRange(taskRuntime->clip, writeback->piecePosition, writeback->pieceLength));
            for (auto currentPiece : current)
                affectedPieces.insert(currentPiece);
        }
        for (auto affected : affectedPieces) {
            if (!affected)
                continue;
            auto d = SynthesisPiecePrivate::get(affected);
            ++d->revision;
            d->state = SynthesisPiece::Stale;
            d->errorMessage.clear();
            d->diagnosticFilePath.clear();
            removeAudio(affected);
            if (tasks.contains(m_pieceTasks.value(affected)))
                m_pieceTasks.remove(affected);
            publishPieceState(affected);
        }
        bool canceled = canceledContinuation;
        for (auto task : tasks) {
            if (task->isFinished()) {
                canceled = true;
            } else {
                canceled = m_taskManager->cancel(task) || canceled;
            }
        }
        finalizeLanguageWave(runtime);
        updateCounts();
        return canceled;
    }

    bool SynthesisProjectAddOn::cancelPieceTask(SynthesisPiece *piece) {
        if (!piece)
            return false;
        auto task = m_pieceTasks.value(piece);
        if (!task || task->isFinished())
            return false;
        return m_taskManager->cancel(task);
    }

    void SynthesisProjectAddOn::cancelAll() {
        for (auto piece : pieces())
            cancelPiece(piece);
        for (auto writeback : m_taskWritebacks) {
            if (!writeback || writeback->discarded || !writeback->task)
                continue;
            writeback->discarded = true;
            if (!writeback->task->isFinished())
                m_taskManager->cancel(writeback->task);
        }
        for (auto runtime : m_clips)
            finalizeLanguageWave(runtime);
        updateCounts();
    }

    void SynthesisProjectAddOn::updatePriorities() {
        if (!m_taskManager || documentTransactionActive())
            return;
        const int playhead = windowHandle()->cast<Core::ProjectWindowInterface>()->projectTimeline()->position();
        const auto priorityForRange = [playhead](double start, double end) {
            if (playhead >= start && playhead < end)
                return 1000000000;
            if (start >= playhead)
                return 500000000 - static_cast<int>(start - playhead);
            return 100000000 - static_cast<int>(playhead - end);
        };
        for (auto writeback : m_taskWritebacks) {
            if (!writeback || writeback->discarded || !writeback->task ||
                writeback->scope != TaskWriteback::Language ||
                writeback->task->state() != SynthesisTask::Queued) {
                continue;
            }
            auto runtime = m_clips.value(writeback->clipHandle);
            if (!runtime || !runtime->clip) {
                continue;
            }
            const double start = runtime->clip->start() + writeback->piecePosition;
            const double end = start + writeback->pieceLength;
            m_taskManager->setPriority(writeback->task, priorityForRange(start, end));
        }
        for (auto it = m_pieceTasks.cbegin(); it != m_pieceTasks.cend(); ++it) {
            auto piece = it.key();
            auto task = it.value();
            if (!piece || !task || task->state() != SynthesisTask::Queued)
                continue;
            if (task->type() <= SynthesisTaskType::Phoneme)
                continue;
            auto clip = piece->singingClip();
            if (!clip) {
                continue;
            }
            const double start = clip->start() + piece->position();
            const double end = start + piece->length();
            m_taskManager->setPriority(task, priorityForRange(start, end));
        }
    }

    void SynthesisProjectAddOn::updateCounts() {
        Q_EMIT m_context->pieceCountsChanged();
    }

    void SynthesisProjectAddOn::publishPieceState(SynthesisPiece *piece) {
        if (!piece) {
            return;
        }
        Q_EMIT piece->stateChanged();
        Q_EMIT m_context->pieceChanged(piece);
    }

    using namespace DocumentWriter;
    using namespace ProjectInput;

    void SynthesisProjectAddOn::queueTaskWriteback(TaskWriteback *writeback) {
        if (!writeback || writeback->queued)
            return;
        writeback->queued = true;
        if (writeback->task && writeback->task->state() == SynthesisTask::Succeeded) {
            const int expected = writeback->noteHandles.size();
            const auto result = writeback->task->result();
            if (writeback->type == SynthesisTaskType::Pronunciation &&
                result.pronunciations.size() != expected) {
                writeback->responseShapeError = tr("The pronunciation result does not match the requested score");
            } else if (writeback->type == SynthesisTaskType::Phoneme && result.phonemes.size() != expected) {
                writeback->responseShapeError = tr("The phoneme result does not match the requested score");
            } else if (writeback->type == SynthesisTaskType::Duration && result.phonemes.size() != expected) {
                writeback->responseShapeError = tr("The duration result does not match the score for the requested synthesis piece");
            }
        }
        m_pendingTaskWritebacks.append(writeback);
        schedulePendingWork();
    }

    void SynthesisProjectAddOn::discardTaskWritebacks(SynthesisTask *task) {
        if (!task)
            return;
        for (auto writeback : m_taskWritebacks) {
            if (writeback && writeback->task == task)
                writeback->discarded = true;
        }
    }

    bool SynthesisProjectAddOn::hasUnprocessedWriteback(SynthesisTask *task) const {
        if (!task)
            return false;
        return std::ranges::any_of(m_taskWritebacks, [task](const TaskWriteback *writeback) {
            return writeback && !writeback->discarded && writeback->task == task;
        });
    }

    bool SynthesisProjectAddOn::validateTaskWriteback(const TaskWriteback *writeback) const {
        if (!writeback || !writeback->task || writeback->discarded || documentTransactionActive())
            return false;
        auto runtime = m_clips.value(writeback->clipHandle);
        if (!runtime || !runtime->clip || !runtime->divider || !isManagedClip(runtime->clip))
            return false;
        const auto context = buildSynthesisContext(runtime->clip);
        if (!context || context->architectureId != writeback->architecture.id())
            return false;
        if (writeback->scope == TaskWriteback::Language) {
            const auto current = buildLanguageRequest(
                runtime->clip, writeback->piecePosition, writeback->pieceLength,
                writeback->type, *context
            );
            return current.noteHandles == writeback->noteHandles &&
                   sameSynthesisInput(current.request, writeback->request) &&
                   !synthesisPiecesIn(runtime, piecesInRange(runtime->clip, writeback->piecePosition, writeback->pieceLength)).isEmpty();
        }
        auto piece = writeback->piece.data();
        if (!piece || piece->singingClip() != runtime->clip ||
            SynthesisPiecePrivate::get(piece)->revision != writeback->revision ||
            m_pieceTasks.value(piece) != writeback->task ||
            piece->position() != writeback->piecePosition ||
            piece->length() != writeback->pieceLength ||
            !isPieceInSynthesisRange(runtime, piece)) {
            return false;
        }
        const auto current = buildScore(
            windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip, piece,
            writeback->architecture, writeback->type == SynthesisTaskType::Audio,
            writeback->requestedParameters
        );
        if (!current.error.isEmpty() || current.noteHandles != writeback->noteHandles)
            return false;
        SynthesisTaskRequest request;
        request.type = writeback->type;
        request.context = *context;
        request.score = current.score;
        request.displayName = runtime->clip->name();
        return sameSynthesisInput(request, writeback->request);
    }

    void SynthesisProjectAddOn::processTaskWriteback(TaskWriteback *writeback) {
        if (!writeback)
            return;
        if (!validateTaskWriteback(writeback)) {
            if (writeback->discarded || !writeback->task ||
                !writeback->task->isFinished() || documentTransactionActive()) {
                return;
            }
            auto runtime = m_clips.value(writeback->clipHandle);
            if (!runtime || !runtime->clip || !runtime->divider || !isManagedClip(runtime->clip))
                return;
            QList<SynthesisPiece *> recoveryPieces = piecesInRange(
                runtime->clip, writeback->piecePosition, writeback->pieceLength
            );
            auto model = runtime->clip->model();
            for (const auto handle : writeback->noteHandles) {
                auto note = model ? model->find<dspx::Note>(handle) : nullptr;
                if (!note || note->noteSequence() != runtime->clip->notes())
                    continue;
                recoveryPieces.append(piecesInRange(
                    runtime->clip, note->position(), std::max(1, note->length())
                ));
            }
            QSet<SynthesisPiece *> uniqueRecovery(
                recoveryPieces.cbegin(), recoveryPieces.cend()
            );
            if (writeback->scope == TaskWriteback::Piece) {
                QList<SynthesisPiece *> orphanPieces;
                for (auto piece : synthesisPiecesIn(runtime, uniqueRecovery.values())) {
                    const auto owner = m_pieceTasks.value(piece);
                    if (owner && owner != writeback->task &&
                        (!owner->isFinished() || hasUnprocessedWriteback(owner))) {
                        continue;
                    }
                    orphanPieces.append(piece);
                }
                if (!orphanPieces.isEmpty()) {
                    invalidate(
                        runtime, writeback->type, orphanPieces, writeback->options,
                        writeback->requestedParameters
                    );
                }
                updateCounts();
                return;
            }
            for (auto piece : synthesisPiecesIn(runtime, uniqueRecovery.values())) {
                TaskWriteback *coverage{};
                for (auto candidate : m_taskWritebacks) {
                    if (!candidate || candidate->discarded || !candidate->task ||
                        candidate->scope != TaskWriteback::Language ||
                        candidate->clipHandle != runtime->clipHandle ||
                        !rangesOverlap(
                            piece->position(), piece->length(),
                            candidate->piecePosition, candidate->pieceLength
                        )) {
                        continue;
                    }
                    if (!coverage || candidate->type < coverage->type ||
                        (candidate->type == coverage->type &&
                         candidate->task->state() == SynthesisTask::Running)) {
                        coverage = candidate;
                    }
                }
                if (coverage) {
                    auto d = SynthesisPiecePrivate::get(piece);
                    d->state = coverage->task->state() == SynthesisTask::Running
                                   ? SynthesisPiece::Synthesizing
                                   : SynthesisPiece::Queued;
                    d->currentTaskType = coverage->type;
                    d->errorMessage.clear();
                    d->diagnosticFilePath.clear();
                    m_pieceTasks.insert(piece, coverage->task);
                    publishPieceState(piece);
                } else {
                    schedulePieceLanguage(
                        runtime, piece, writeback->type, writeback->options
                    );
                }
            }
            updateCounts();
            return;
        }
        auto runtime = m_clips.value(writeback->clipHandle);
        auto task = writeback->task.data();
        auto piece = writeback->piece.data();
        const auto fail = [this, runtime, piece, writeback, task](const QString &message) {
            const auto diagnosticFilePath = task ? task->diagnosticFilePath() : QString();
            if (writeback->scope == TaskWriteback::Language) {
                runtime->languageContinuations.append({
                    writeback->piecePosition,
                    writeback->pieceLength,
                    writeback->options,
                    writeback->architecture,
                    true,
                    false,
                    message,
                });
                const auto affectedPieces = synthesisPiecesIn(runtime, piecesInRange(runtime->clip, writeback->piecePosition, writeback->pieceLength));
                for (auto affected : affectedPieces) {
                    for (auto candidate : m_taskWritebacks) {
                        if (!candidate || candidate->discarded ||
                            candidate->scope != TaskWriteback::Language ||
                            candidate->clipHandle != runtime->clipHandle ||
                            !rangesOverlap(
                                affected->position(), affected->length(),
                                candidate->piecePosition, candidate->pieceLength
                            )) {
                            continue;
                        }
                        if (candidate != writeback) {
                            runtime->languageContinuations.append({
                                candidate->piecePosition,
                                candidate->pieceLength,
                                candidate->options,
                                candidate->architecture,
                                true,
                                false,
                                message,
                            });
                        }
                        candidate->discarded = true;
                        if (candidate->task && candidate->task->state() == SynthesisTask::Queued)
                            m_taskManager->cancel(candidate->task);
                    }
                    notifyFailure(affected, message, diagnosticFilePath);
                }
                return;
            }
            notifyFailure(piece, message, diagnosticFilePath);
        };
        if (task->state() != SynthesisTask::Succeeded) {
            fail(task->errorMessage().isEmpty() ? tr("Synthesis was canceled") : task->errorMessage());
            return;
        }
        if (!writeback->responseShapeError.isEmpty()) {
            fail(writeback->responseShapeError);
            return;
        }

        auto model = runtime->clip->model();
        QList<dspx::Note *> notes;
        notes.reserve(writeback->noteHandles.size());
        for (const auto handle : writeback->noteHandles) {
            auto note = model->find<dspx::Note>(handle);
            if (!note || note->noteSequence() != runtime->clip->notes())
                return;
            notes.append(note);
        }
        const auto result = task->result();
        switch (writeback->type) {
            case SynthesisTaskType::Pronunciation:
                processPronunciationWriteback(runtime, writeback, notes, result);
                break;
            case SynthesisTaskType::Phoneme:
                processPhonemeWriteback(runtime, writeback, notes, result);
                break;
            case SynthesisTaskType::Duration:
                processDurationWriteback(runtime, writeback, notes, result);
                break;
            case SynthesisTaskType::Parameter:
                processParameterWriteback(runtime, writeback, result);
                break;
            case SynthesisTaskType::Audio:
                processAudioWriteback(runtime, writeback, result);
                break;
        }
    }

    void SynthesisProjectAddOn::processPronunciationWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const QList<dspx::Note *> &notes, const SynthesisTaskResult &result) {
        for (int index = 0; index < notes.size(); ++index) {
            notes.at(index)->setOriginalPronunciation(result.pronunciations.at(index).pronunciation);
        }
        scheduleLanguageRange(
            runtime, writeback->piecePosition, writeback->pieceLength,
            nextStage(writeback->architecture, SynthesisTaskType::Pronunciation), writeback->options
        );
    }

    void SynthesisProjectAddOn::processPhonemeWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const QList<dspx::Note *> &notes, const SynthesisTaskResult &result) {
        for (int index = 0; index < notes.size(); ++index) {
            replaceOriginalPhonemes(notes.at(index), result.phonemes.at(index));
        }
        runtime->languageContinuations.append({
            writeback->piecePosition,
            writeback->pieceLength,
            writeback->options,
            writeback->architecture,
            false,
            false,
            {},
        });
        const auto next = nextStage(writeback->architecture, SynthesisTaskType::Phoneme);
        for (auto affected : synthesisPiecesIn(runtime, piecesInRange(runtime->clip, writeback->piecePosition, writeback->pieceLength))) {
            auto d = SynthesisPiecePrivate::get(affected);
            d->state = SynthesisPiece::Queued;
            d->currentTaskType = next;
            d->errorMessage.clear();
            d->diagnosticFilePath.clear();
            if (m_pieceTasks.value(affected) == writeback->task) {
                m_pieceTasks.remove(affected);
            }
            publishPieceState(affected);
        }
        updateCounts();
    }

    void SynthesisProjectAddOn::processDurationWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const QList<dspx::Note *> &notes, const SynthesisTaskResult &result) {
        for (int index = 0; index < notes.size(); ++index) {
            replaceOriginalPhonemes(notes.at(index), result.phonemes.at(index));
        }
        schedulePieceStage(runtime, writeback->piece, nextStage(writeback->architecture, SynthesisTaskType::Duration), writeback->options);
    }

    void SynthesisProjectAddOn::processParameterWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const SynthesisTaskResult &result) {
        if (!ensureParameterNodes(runtime, result.parameters.keys()) || !validateTaskWriteback(writeback)) {
            return;
        }
        writeParameterOrigins(
            windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip,
            writeback->piece, result.parameters
        );
        schedulePieceStage(runtime, writeback->piece, nextStage(writeback->architecture, SynthesisTaskType::Parameter), writeback->options);
    }

    void SynthesisProjectAddOn::processAudioWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const SynthesisTaskResult &result) {
        QString error;
        if (!installAudio(runtime, writeback->piece, result.audioFilePath, &error)) {
            notifyFailure(writeback->piece, error);
        }
    }

    void SynthesisProjectAddOn::processManualRequest(ManualRequest *request) {
        if (!request)
            return;
        if (request->scope == ManualRequest::Project) {
            resynthesizeProject(request->fromType, request->options);
            return;
        }
        auto runtime = m_clips.value(request->clipHandle);
        if (!runtime || !runtime->clip)
            return;
        if (request->scope == ManualRequest::Clip) {
            resynthesizeClip(runtime->clip, request->fromType, request->options);
            return;
        }
        const auto affected = piecesInRange(runtime->clip, request->position, request->length);
        if (affected.isEmpty())
            return;
        for (auto piece : affected)
            resynthesizePiece(piece, request->fromType, request->options);
    }

    void SynthesisProjectAddOn::processCommittedChanges() {
        if (documentTransactionActive()) {
            m_commitPending = true;
            schedulePendingWork();
            return;
        }
        const bool globalCentShiftChanged = std::exchange(m_globalCentShiftPending, false);
        const auto addedClips = synchronizeDocument();
        const QSet<dspx::SingingClip *> addedClipSet(addedClips.cbegin(), addedClips.cend());
        for (auto runtime : m_clips) {
            if (!runtime || !runtime->clip || !runtime->divider || !runtime->watcher) {
                continue;
            }
            if (addedClipSet.contains(runtime->clip))
                continue;
            const bool rebound = std::exchange(runtime->rebound, false);
            const auto currentContext = buildSynthesisContext(runtime->clip);
            const bool synthesisContextChanged = currentContext != runtime->synthesisContextSnapshot;
            runtime->synthesisContextSnapshot = currentContext;
            const auto change = runtime->watcher->takeChanges();
            if (!rebound && !synthesisContextChanged && change.isEmpty() && !globalCentShiftChanged)
                continue;
            SynthesisTaskType from = SynthesisTaskType::Audio;
            if (rebound || synthesisContextChanged || change.contains(dspx::ClipChange::Sources) || change.contains(dspx::ClipChange::Lyric) ||
                change.contains(dspx::ClipChange::Score) || change.contains(dspx::ClipChange::ClipTiming)) {
                from = SynthesisTaskType::Pronunciation;
            } else if (change.contains(dspx::ClipChange::Pronunciation)) {
                from = SynthesisTaskType::Phoneme;
            } else if (change.contains(dspx::ClipChange::Note) || change.contains(dspx::ClipChange::Phoneme)) {
                from = SynthesisTaskType::Duration;
            } else if (change.contains(dspx::ClipChange::Vibrato) || change.contains(dspx::ClipChange::Parameter)) {
                from = SynthesisTaskType::Parameter;
            }
            if (globalCentShiftChanged && from > SynthesisTaskType::Duration) {
                from = SynthesisTaskType::Duration;
            }
            const auto affectedPieces = [this, runtime, &change, globalCentShiftChanged, synthesisContextChanged] {
                if (synthesisContextChanged || globalCentShiftChanged || change.ranges().isEmpty()) {
                    return piecesForClip(runtime->clip);
                }
                QList<SynthesisPiece *> result;
                for (const auto &range : change.ranges()) {
                    result.append(piecesInRange(runtime->clip, range.position(), range.length()));
                }
                QSet<SynthesisPiece *> unique(result.cbegin(), result.cend());
                return unique.values();
            };

            const auto oldAffected = affectedPieces();
            QList<QPair<double, double>> invalidatedRanges;
            for (auto piece : oldAffected) {
                if (piece) {
                    invalidatedRanges.append({piece->position(), piece->length()});
                }
            }
            QList<QPair<double, double>> languageTopologyRanges;
            for (const auto &continuation : runtime->languageContinuations) {
                if (continuation.failed)
                    continue;
                for (auto piece : piecesInRange(
                         runtime->clip, continuation.position, continuation.length
                     )) {
                    if (piece)
                        languageTopologyRanges.append({piece->position(), piece->length()});
                }
            }
            for (auto writeback : m_taskWritebacks) {
                if (!writeback || writeback->discarded ||
                    writeback->scope != TaskWriteback::Language ||
                    writeback->clipHandle != runtime->clipHandle) {
                    continue;
                }
                for (auto piece : piecesInRange(
                         runtime->clip, writeback->piecePosition, writeback->pieceLength
                     )) {
                    if (piece)
                        languageTopologyRanges.append({piece->position(), piece->length()});
                }
            }
            std::optional<QStringList> requestedParameters;
            if (from == SynthesisTaskType::Parameter &&
                change.contains(dspx::ClipChange::Parameter) &&
                !change.contains(dspx::ClipChange::Vibrato) &&
                !change.parameterNames().isEmpty()) {
                const auto context = buildSynthesisContext(runtime->clip);
                const auto architecture = context ? architectureFor(*context) : ArchitectureMetadata{};
                if (!architecture.id().isEmpty()) {
                    requestedParameters = downstreamIndirectParameters(architecture, change.parameterNames());
                }
            }

            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
            auto affected = affectedPieces();
            for (const auto &[position, length] : invalidatedRanges)
                affected.append(piecesInRange(runtime->clip, position, length));
            QSet<SynthesisPiece *> uniqueAffected(affected.cbegin(), affected.cend());
            affected = uniqueAffected.values();
            for (auto piece : affected) {
                if (piece) {
                    invalidatedRanges.append({piece->position(), piece->length()});
                }
            }
            for (const auto &[position, length] : languageTopologyRanges)
                affected.append(piecesInRange(runtime->clip, position, length));
            uniqueAffected = QSet<SynthesisPiece *>(affected.cbegin(), affected.cend());
            affected = uniqueAffected.values();
            if (from <= SynthesisTaskType::Parameter) {
                clearParameterOrigins(runtime->clip, invalidatedRanges, requestedParameters);
            }
            invalidate(runtime, from, affected, {}, requestedParameters);
            QList<SynthesisPiece *> idlePieces;
            for (auto piece : synthesisPiecesForClip(runtime)) {
                if (piece && piece->state() == SynthesisPiece::Idle)
                    idlePieces.append(piece);
            }
            if (!idlePieces.isEmpty())
                invalidate(runtime, SynthesisTaskType::Pronunciation, idlePieces, {});
        }
        for (auto clip : addedClips)
            resynthesizeClip(clip, SynthesisTaskType::Pronunciation, {});
        updatePriorities();
    }

    void SynthesisProjectAddOn::finalizeLanguageWave(ClipRuntime *runtime) {
        if (!runtime || m_clips.value(runtime->clipHandle) != runtime ||
            !runtime->clip || !runtime->divider || documentTransactionActive()) {
            return;
        }
        const bool hasActiveLanguageTask = std::ranges::any_of(
            m_taskWritebacks, [runtime](const TaskWriteback *writeback) {
                return writeback && !writeback->discarded && writeback->task &&
                       writeback->scope == TaskWriteback::Language &&
                       writeback->clipHandle == runtime->clipHandle;
            }
        );
        if (hasActiveLanguageTask || runtime->languageContinuations.isEmpty()) {
            return;
        }

        const auto continuations = std::exchange(
            runtime->languageContinuations, QList<ClipRuntime::LanguageContinuation>{}
        );
        const bool hasSuccessfulPhoneme = std::ranges::any_of(
            continuations, [](const ClipRuntime::LanguageContinuation &continuation) {
                return !continuation.failed;
            }
        );
        QHash<SynthesisPiece *, QPair<double, double>> oldRanges;
        for (auto piece : runtime->pieces) {
            if (piece)
                oldRanges.insert(piece, {piece->position(), piece->length()});
        }
        if (hasSuccessfulPhoneme) {
            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
        }

        for (auto piece : synthesisPiecesForClip(runtime)) {
            const auto oldRange = oldRanges.constFind(piece);
            const bool topologyChanged = oldRange == oldRanges.cend() ||
                                         oldRange->first != piece->position() ||
                                         oldRange->second != piece->length();
            const ClipRuntime::LanguageContinuation *successful{};
            const ClipRuntime::LanguageContinuation *failed{};
            for (const auto &continuation : continuations) {
                if (!rangesOverlap(
                        piece->position(), piece->length(),
                        continuation.position, continuation.length
                    )) {
                    continue;
                }
                if (continuation.failed || continuation.canceled) {
                    failed = &continuation;
                } else {
                    successful = &continuation;
                }
            }
            if (topologyChanged && !successful && !failed) {
                double nearestDistance = std::numeric_limits<double>::max();
                const double pieceEnd = piece->position() + piece->length();
                const ClipRuntime::LanguageContinuation *nearest{};
                for (const auto &continuation : continuations) {
                    const double continuationEnd = continuation.position + continuation.length;
                    const double distance = pieceEnd <= continuation.position
                                                ? continuation.position - pieceEnd
                                            : continuationEnd <= piece->position()
                                                ? piece->position() - continuationEnd
                                                : 0.0;
                    if (distance < nearestDistance) {
                        nearestDistance = distance;
                        nearest = &continuation;
                    }
                }
                if (nearest && (nearest->failed || nearest->canceled)) {
                    failed = nearest;
                } else {
                    successful = nearest;
                }
            }
            if (failed) {
                m_pieceTasks.remove(piece);
                if (failed->errorMessage.isEmpty()) {
                    auto d = SynthesisPiecePrivate::get(piece);
                    ++d->revision;
                    d->state = SynthesisPiece::Stale;
                    d->errorMessage.clear();
                    d->diagnosticFilePath.clear();
                    removeAudio(piece);
                    publishPieceState(piece);
                } else {
                    notifyFailure(piece, failed->errorMessage);
                }
                continue;
            }
            if (!successful) {
                continue;
            }
            if (topologyChanged) {
                auto d = SynthesisPiecePrivate::get(piece);
                ++d->revision;
                d->state = SynthesisPiece::Stale;
                d->errorMessage.clear();
                d->diagnosticFilePath.clear();
                removeAudio(piece);
            }
            const auto next = nextStage(successful->architecture, SynthesisTaskType::Phoneme);
            schedulePieceStage(runtime, piece, next, successful->options);
        }
        updatePriorities();
        updateCounts();
    }

}

#include "moc_SynthesisProjectAddOn.cpp"
