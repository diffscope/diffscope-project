#ifndef DIFFSCOPE_VISUALEDITOR_TEMPOSELECTIONCONTROLLER_P_H
#define DIFFSCOPE_VISUALEDITOR_TEMPOSELECTIONCONTROLLER_P_H

#include <ScopicFlowCore/SelectionController.h>

class QQuickItem;

namespace dspx {
    class SelectionModel;
    class Tempo;
    class TempoSelectionModel;
    class TempoSequence;
}

namespace VisualEditor {

    class ProjectViewModelContext;

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

}

#endif // DIFFSCOPE_VISUALEDITOR_TEMPOSELECTIONCONTROLLER_P_H
