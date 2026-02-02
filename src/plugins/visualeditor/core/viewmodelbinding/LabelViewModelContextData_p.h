#ifndef DIFFSCOPE_VISUALEDITOR_LABELVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_LABELVIEWMODELCONTEXTDATA_P_H

#include <QHash>
#include <QSet>

#include <transactional/TransactionController.h>
#include <visualeditor/ProjectViewModelContext.h>

class QStateMachine;
class QState;
class QQuickItem;

namespace dspx {
    class Label;
    class LabelSelectionModel;
    class LabelSequence;
}

namespace sflow {
    class LabelSequenceInteractionController;
    class LabelViewModel;
    class PointSequenceViewModel;
}

namespace Core {
    class DspxDocument;
}

namespace VisualEditor {

    class LabelSelectionController;

    class LabelViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::DspxDocument *document;
        dspx::LabelSequence *labelSequence;
        dspx::LabelSelectionModel *labelSelectionModel;

        sflow::PointSequenceViewModel *labelSequenceViewModel;
        LabelSelectionController *labelSelectionController;

        QHash<dspx::Label *, sflow::LabelViewModel *> labelViewItemMap;
        QHash<sflow::LabelViewModel *, dspx::Label *> labelDocumentItemMap;
        QSet<sflow::LabelViewModel *> transactionalUpdatedLabels;
        bool shouldCopyBeforeMove{};

        QStateMachine *stateMachine;
        QState *idleState;
        QState *movePendingState;
        QState *moveProgressingState;
        QState *moveCommittingState;
        QState *moveAbortingState;
        QState *rubberBandDraggingState;
        QState *editPendingState;
        QState *editProgressingState;
        QState *editCommittingState;
        QState *editAbortingState;
        QState *insertPendingState;
        QState *insertProgressingState;
        QState *insertCommittingState;
        QState *insertAbortingState;

        QQuickItem *targetLabelSequenceItem;
        int targetPosition{};
        dspx::Label *targetDocumentItem{};

        Core::TransactionController::TransactionId moveTransactionId{};
        Core::TransactionController::TransactionId editTransactionId{};
        Core::TransactionController::TransactionId insertTransactionId{};

        void init();
        void initStateMachine();
        void bindLabelSequenceViewModel();
        void bindLabelDocumentItem(dspx::Label *item);
        void unbindLabelDocumentItem(dspx::Label *item);
        sflow::LabelSequenceInteractionController *createController(QObject *parent);

        void onMovePendingStateEntered();
        void onMoveCommittingStateEntered();
        void onMoveAbortingStateEntered();
        void onEditPendingStateEntered();
        void onEditCommittingStateEntered();
        void onEditAbortingStateEntered();
        void onInsertPendingStateEntered();
        void onInsertCommittingStateEntered();
        void onInsertAbortingStateEntered();
        void onDoubleClicked(QQuickItem *labelSequenceItem, int position);
        void onItemDoubleClicked(QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem);

    Q_SIGNALS:
        void moveTransactionWillStart();
        void moveTransactionStarted();
        void moveTransactionNotStarted();
        void moveTransactionWillCommit();
        void moveTransactionWillAbort();

        void rubberBandDragWillStart();
        void rubberBandDragWillFinish();

        void editTransactionWillStart();
        void editTransactionStarted();
        void editTransactionNotStarted();
        void insertTransactionWillStart();
        void insertTransactionStarted();
        void insertTransactionNotStarted();
        void editOrInsertTransactionWillCommit();
        void editOrInsertTransactionWillAbort();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_LABELVIEWMODELCONTEXTDATA_P_H
