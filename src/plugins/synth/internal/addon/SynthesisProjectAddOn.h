#ifndef DIFFSCOPE_SYNTH_SYNTHESISPROJECTADDON_H
#define DIFFSCOPE_SYNTH_SYNTHESISPROJECTADDON_H

#include <optional>

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QStringList>

#include <CoreApi/windowinterface.h>

#include <dini/event.h>
#include <synth/SynthesisModel.h>

namespace dspx {
    class Piece;
    class SingingClip;
}

namespace Synth {

    class ProjectSynthesisContext;
    class SynthesisPiece;
    class SynthesisTask;
    class SynthesisTaskManager;

    namespace Internal {

        class SynthesisProjectAddOn final : public Core::WindowInterfaceAddOn {
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

            void resynthesizeProject(SynthesisTaskType fromType, const SynthesisTaskOptions &options);
            void resynthesizeClip(dspx::SingingClip *clip, SynthesisTaskType fromType, const SynthesisTaskOptions &options);
            void resynthesizePiece(SynthesisPiece *piece, SynthesisTaskType fromType, const SynthesisTaskOptions &options);
            bool cancelPiece(SynthesisPiece *piece);
            void cancelAll();

        private:
            struct ClipRuntime;
            struct AudioBinding;

            void synchronizeDocument();
            void addClip(dspx::SingingClip *clip);
            void removeClip(dspx::SingingClip *clip);
            void synchronizePieces(ClipRuntime *runtime);
            void processCommittedChanges();
            void invalidate(ClipRuntime *runtime, SynthesisTaskType fromType, const QList<SynthesisPiece *> &pieces, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters = std::nullopt);
            void scheduleClipLanguage(ClipRuntime *runtime, SynthesisTaskType fromType, const SynthesisTaskOptions &options);
            void schedulePieceStage(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTaskType type, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters = std::nullopt);
            void bindPieceTask(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTask *task, quint64 revision, const SynthesisTaskOptions &options);
            void updatePriorities();
            void updateCounts();
            bool ensureParameterNodes(ClipRuntime *runtime, const QStringList &parameterIds);
            void removeAudio(SynthesisPiece *piece);
            bool installAudio(ClipRuntime *runtime, SynthesisPiece *piece, const QString &filePath, QString *errorMessage);
            void notifyFailure(SynthesisPiece *piece, const QString &message);

            ProjectSynthesisContext *m_context{};
            SynthesisTaskManager *m_taskManager{};
            QHash<dspx::SingingClip *, ClipRuntime *> m_clips;
            QHash<SynthesisPiece *, SynthesisTask *> m_pieceTasks;
            QHash<SynthesisPiece *, AudioBinding *> m_audioBindings;
            QList<ClipRuntime *> m_retiredClips;
            dini::Subscription m_subscription;
            bool m_commitPending{};
            bool m_internalCommit{};
            QHash<QString, QDateTime> m_lastNotifications;
        };

    }

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPROJECTADDON_H
