// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISPROJECTADDON_H
#define DIFFSCOPE_SYNTH_SYNTHESISPROJECTADDON_H

#include <functional>
#include <optional>

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QSet>
#include <QStringList>

#include <CoreApi/windowinterface.h>

#include <dini/event.h>
#include <dspxmodelORM/Handle.h>
#include <synth/SynthesisModel.h>

namespace Core {
    class TransactionController;
}

namespace dspx {
    class Note;
    class Piece;
    class SingingClip;
}

namespace Synth {

    class ProjectSynthesisContext;
    class SynthesisPiece;
    class SynthesisTask;
    class SynthesisTaskManager;

    namespace Internal {

        class SynthesisExportListener;
        class SynthesisAudioController;

        class SynthesisProjectAddOn : public Core::WindowInterfaceAddOn {
            Q_OBJECT
            Q_PROPERTY(Synth::ProjectSynthesisContext *synthesisContext READ synthesisContext CONSTANT)

        public:
            explicit SynthesisProjectAddOn(QObject *parent = nullptr);
            ~SynthesisProjectAddOn() override;

            void initialize() override;
            void extensionsInitialized() override;
            bool delayedInitialize() override;

            ProjectSynthesisContext *synthesisContext() const;
            QList<SynthesisPiece *> pieces() const;
            QList<SynthesisPiece *> piecesForClip(dspx::SingingClip *clip) const;
            QList<SynthesisPiece *> piecesInRange(dspx::SingingClip *clip, double position, double length) const;
            int synthesizingPieceCount() const;
            int queuedPieceCount() const;

            Q_INVOKABLE void resynthesizeSelectedItems();
            void resynthesizeProject(SynthesisTaskType fromType, const SynthesisTaskOptions &options);
            void resynthesizeClip(dspx::SingingClip *clip, SynthesisTaskType fromType, const SynthesisTaskOptions &options);
            void resynthesizePiece(SynthesisPiece *piece, SynthesisTaskType fromType, const SynthesisTaskOptions &options);
            bool cancelPiece(SynthesisPiece *piece);
            bool cancelPieceTask(SynthesisPiece *piece);
            void cancelAll();

        private:
            struct ClipRuntime;
            struct TaskWriteback;
            struct ManualRequest;

            friend class SynthesisExportListener;

            QList<dspx::SingingClip *> synchronizeDocument();
            ClipRuntime *runtimeForClip(dspx::SingingClip *clip) const;
            ClipRuntime *runtimeForPiece(SynthesisPiece *piece) const;
            void addClip(dspx::SingingClip *clip);
            void rebindClip(ClipRuntime *runtime, dspx::SingingClip *clip);
            void removeClip(dspx::Handle clipHandle);
            void watchClipLifetime(ClipRuntime *runtime);
            void resetClipBaseline(ClipRuntime *runtime);
            void synchronizePieces(ClipRuntime *runtime);
            bool isPieceInSynthesisRange(const ClipRuntime *runtime, const SynthesisPiece *piece) const;
            QList<SynthesisPiece *> synthesisPiecesForClip(const ClipRuntime *runtime) const;
            QList<SynthesisPiece *> synthesisPiecesIn(const ClipRuntime *runtime, const QList<SynthesisPiece *> &pieces) const;
            void deactivatePiece(SynthesisPiece *piece);
            bool documentTransactionActive() const;
            void schedulePendingWork();
            void processPendingWork();
            void queueFinalizer(std::function<void()> finalizer);
            void queueTaskWriteback(TaskWriteback *writeback);
            void discardTaskWritebacks(SynthesisTask *task);
            bool hasUnprocessedWriteback(SynthesisTask *task) const;
            bool validateTaskWriteback(const TaskWriteback *writeback) const;
            void processTaskWriteback(TaskWriteback *writeback);
            void processPronunciationWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const QList<dspx::Note *> &notes, const SynthesisTaskResult &result);
            void processPhonemeWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const QList<dspx::Note *> &notes, const SynthesisTaskResult &result);
            void processDurationWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const QList<dspx::Note *> &notes, const SynthesisTaskResult &result);
            void processParameterWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const SynthesisTaskResult &result);
            void processAudioWriteback(ClipRuntime *runtime, TaskWriteback *writeback, const SynthesisTaskResult &result);
            void processManualRequest(ManualRequest *request);
            void processCommittedChanges();
            void finalizeLanguageWave(ClipRuntime *runtime);
            void invalidate(ClipRuntime *runtime, SynthesisTaskType fromType, const QList<SynthesisPiece *> &pieces, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters = std::nullopt);
            void schedulePieceLanguage(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTaskType fromType, const SynthesisTaskOptions &options);
            void scheduleLanguageRange(ClipRuntime *runtime, double position, double length, SynthesisTaskType fromType, const SynthesisTaskOptions &options);
            void schedulePieceStage(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTaskType type, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters = std::nullopt);
            void bindPieceTask(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTask *task, quint64 revision, const SynthesisTaskOptions &options);
            void updatePriorities();
            void updateCounts();
            void publishPieceState(SynthesisPiece *piece);
            bool ensureParameterNodes(ClipRuntime *runtime, const QStringList &parameterIds);
            bool prepareAudio(ClipRuntime *runtime, SynthesisPiece *piece, QString *errorMessage = nullptr);
            void removeAudio(SynthesisPiece *piece);
            void detachAudioSeries(ClipRuntime *runtime);
            bool installAudio(ClipRuntime *runtime, SynthesisPiece *piece, const QString &filePath, QString *errorMessage);
            bool refreshAudioRanges(double sampleRate, QString *errorMessage = nullptr);
            bool waitForAudioSynthesis(QString *errorMessage);
            void notifyFailure(SynthesisPiece *piece, const QString &message, const QString &diagnosticFilePath = {});

            ProjectSynthesisContext *m_context{};
            SynthesisTaskManager *m_taskManager{};
            SynthesisAudioController *m_audioController{};
            QPointer<Core::TransactionController> m_transactionController;
            QHash<dspx::Handle, ClipRuntime *> m_clips;
            QHash<SynthesisPiece *, SynthesisTask *> m_pieceTasks;
            QList<ClipRuntime *> m_retiredClips;
            QSet<TaskWriteback *> m_taskWritebacks;
            QList<TaskWriteback *> m_pendingTaskWritebacks;
            QList<ManualRequest *> m_pendingManualRequests;
            QList<std::function<void()>> m_pendingFinalizers;
            dini::Subscription m_subscription;
            bool m_commitPending{};
            bool m_internalCommit{};
            bool m_documentSyncPending{};
            bool m_rollbackPending{};
            bool m_architectureSyncPending{};
            bool m_globalCentShiftPending{};
            bool m_pendingWorkScheduled{};
            bool m_processingPendingWork{};
            QHash<QString, QDateTime> m_lastNotifications;
        };

    }

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPROJECTADDON_H
