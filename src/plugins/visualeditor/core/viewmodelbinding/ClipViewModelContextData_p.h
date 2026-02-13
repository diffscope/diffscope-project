#ifndef DIFFSCOPE_VISUALEDITOR_CLIPVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_CLIPVIEWMODELCONTEXTDATA_P_H

#include <QHash>
#include <QSet>

#include <transactional/TransactionController.h>
#include <visualeditor/ProjectViewModelContext.h>

class QState;
class QStateMachine;
class QQuickItem;

namespace dspx {
    class Clip;
    class ClipSelectionModel;
    class SelectionModel;
    class Track;
    class TrackList;
}

namespace sflow {
    class ClipPaneInteractionController;
    class ClipViewModel;
    class RangeSequenceViewModel;
}

namespace Core {
    class DspxDocument;
}

namespace VisualEditor {

    class ClipSelectionController;

    class ClipViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::DspxDocument *document;
        dspx::TrackList *trackList;
        dspx::SelectionModel *selectionModel;
        dspx::ClipSelectionModel *clipSelectionModel;

        sflow::RangeSequenceViewModel *clipSequenceViewModel;
        ClipSelectionController *clipSelectionController;

        QHash<dspx::Clip *, sflow::ClipViewModel *> clipViewItemMap;
        QHash<sflow::ClipViewModel *, dspx::Clip *> clipDocumentItemMap;
        QSet<sflow::ClipViewModel *> moveUpdatedClips;
        QSet<sflow::ClipViewModel *> lengthUpdatedClips;

        bool shouldCopyBeforeMove{};
        bool moveChanged{};
        bool lengthChanged{};
        bool drawChanged{};

        dspx::Clip *targetClip{};
        QQuickItem *targetClipPane{};

        Core::TransactionController::TransactionId moveTransactionId{};
        Core::TransactionController::TransactionId adjustTransactionId{};
        Core::TransactionController::TransactionId drawTransactionId{};

        QStateMachine *stateMachine;
        QState *idleState;
        QState *rubberBandDraggingState;
        QState *movePendingState;
        QState *moveProcessingState;
        QState *moveCommittingState;
        QState *moveAbortingState;
        QState *adjustPendingState;
        QState *adjustProcessingState;
        QState *adjustCommittingState;
        QState *adjustAbortingState;
        QState *drawPendingState;
        QState *drawProcessingState;
        QState *drawCommittingState;
        QState *drawAbortingState;
        QState *splitPendingState;
        QState *splitCommittingState;

        int splitPosition{};

        void init();
        void initStateMachine();
        void bindClipSequences();
        void bindTrack(dspx::Track *track);
        void unbindTrack(dspx::Track *track);
        void bindClipDocumentItem(dspx::Clip *item);
        void unbindClipDocumentItem(dspx::Clip *item);
        sflow::ClipPaneInteractionController *createController(QObject *parent);

        void onMovePendingStateEntered();
        void onMoveCommittingStateEntered();
        void onMoveAbortingStateEntered();

        void onAdjustPendingStateEntered();
        void onAdjustCommittingStateEntered();
        void onAdjustAbortingStateEntered();

        void onDrawPendingStateEntered();
        void onDrawCommittingStateEntered();
        void onDrawAbortingStateEntered();

        void onSplitCommittingStateEntered();

    Q_SIGNALS:
        void rubberBandDragWillStart();
        void rubberBandDragWillFinish();

        void moveTransactionWillStart();
        void moveTransactionStarted();
        void moveTransactionNotStarted();
        void moveTransactionWillCommit();
        void moveTransactionWillAbort();

        void adjustTransactionWillStart();
        void adjustTransactionStarted();
        void adjustTransactionNotStarted();
        void adjustTransactionWillCommit();
        void adjustTransactionWillAbort();

        void drawTransactionWillStart();
        void drawTransactionStarted();
        void drawTransactionNotStarted();
        void drawTransactionWillCommit();
        void drawTransactionWillAbort();

        void splitWillStart();
        void splitWillCommit();
        void splitWillAbort();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_CLIPVIEWMODELCONTEXTDATA_P_H
