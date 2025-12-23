#ifndef DIFFSCOPE_VISUALEDITOR_TEMPOVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_TEMPOVIEWMODELCONTEXTDATA_P_H

#include <QHash>
#include <QSet>

#include <ScopicFlowCore/SelectionController.h>

#include <visualeditor/ProjectViewModelContext.h>

class QStateMachine;
class QState;

namespace dspx {
    class TempoSequence;
    class TempoSelectionModel;
    class SelectionModel;
}

namespace VisualEditor {

    class TempoSelectionController : public sflow::SelectionController {
        Q_OBJECT
    public:
        explicit TempoSelectionController(ProjectViewModelContext *parent);
        ~TempoSelectionController() override;

        QObjectList getSelectedItems() const override;
        QObjectList getItemsBetween(QObject *startItem, QObject *endItem) const override;
        void select(QObject *item, SelectionCommand command) override;
        QObject *currentItem() const override;

    private:
        ProjectViewModelContext *q;
        dspx::TempoSequence *tempoSequence;
        dspx::SelectionModel *selectionModel;
        dspx::TempoSelectionModel *tempoSelectionModel;
    };

    class TempoViewModelContextData {
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        sflow::PointSequenceViewModel *tempoSequenceViewModel;

        QHash<dspx::Tempo *, sflow::LabelViewModel *> tempoViewItemMap;
        QHash<sflow::LabelViewModel *, dspx::Tempo *> tempoDocumentItemMap;
        QSet<sflow::LabelViewModel *> transactionalUpdatedTempos;

        TempoSelectionController *tempoSelectionController;

        dspx::TempoSequence *tempoSequence;
        dspx::TempoSelectionModel *tempoSelectionModel;

        QStateMachine *stateMachine;
        QState *idleState;
        QState *movingState;
        QState *rubberBandDraggingState;

        void init();
        void bindTempoSequenceViewModel();
        void bindTempoDocumentItem(dspx::Tempo *item);
        void unbindTempoDocumentItem(dspx::Tempo *item);
        sflow::LabelSequenceInteractionController *createController(QObject *parent);
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_TEMPOVIEWMODELCONTEXTDATA_P_H
