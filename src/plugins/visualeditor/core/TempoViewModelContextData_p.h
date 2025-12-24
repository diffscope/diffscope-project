#ifndef DIFFSCOPE_VISUALEDITOR_TEMPOVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_TEMPOVIEWMODELCONTEXTDATA_P_H

#include <QHash>
#include <QSet>

#include <transactional/TransactionController.h>
#include <visualeditor/ProjectViewModelContext.h>

class QStateMachine;
class QState;
class QQuickItem;

namespace dspx {
    class TempoSequence;
    class TempoSelectionModel;
    class SelectionModel;
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

    class TempoSelectionController;

    class TempoViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        // Cached document objects
        Core::DspxDocument *document;
        dspx::TempoSequence *tempoSequence;
        dspx::TempoSelectionModel *tempoSelectionModel;

        sflow::PointSequenceViewModel *tempoSequenceViewModel;
        TempoSelectionController *tempoSelectionController;

        QHash<dspx::Tempo *, sflow::LabelViewModel *> tempoViewItemMap;
        QHash<sflow::LabelViewModel *, dspx::Tempo *> tempoDocumentItemMap;
        QSet<sflow::LabelViewModel *> transactionalUpdatedTempos;

        QStateMachine *stateMachine;
        QState *idleState;
        QState *movePendingState;
        QState *movingState;
        QState *rubberBandDraggingState;

        Core::TransactionController::TransactionId moveTransactionId{};

        void init();
        void initStateMachine();
        void bindTempoSequenceViewModel();
        void bindTempoDocumentItem(dspx::Tempo *item);
        void unbindTempoDocumentItem(dspx::Tempo *item);
        sflow::LabelSequenceInteractionController *createController(QObject *parent);

        void onMovePendingStateEntered();
        void onMovingStateExited();
        void onDoubleClicked(QQuickItem *labelSequenceItem, int position);
        void onItemDoubleClicked(QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem);

    Q_SIGNALS:
        void moveTransactionWillStart();
        void moveTransactionStarted();
        void moveTransactionNotStarted();
        void moveTransactionWillFinish();
        void rubberBandDragWillStart();
        void rubberBandDragWillFinish();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_TEMPOVIEWMODELCONTEXTDATA_P_H
