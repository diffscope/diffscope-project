#ifndef DIFFSCOPE_VISUALEDITOR_LABELSELECTIONCONTROLLER_P_H
#define DIFFSCOPE_VISUALEDITOR_LABELSELECTIONCONTROLLER_P_H

#include <ScopicFlowCore/SelectionController.h>

class QQuickItem;

namespace dspx {
    class SelectionModel;
    class Label;
    class LabelSelectionModel;
    class LabelSequence;
}

namespace VisualEditor {

    class ProjectViewModelContext;

    class LabelSelectionController : public sflow::SelectionController {
        Q_OBJECT
    public:
        explicit LabelSelectionController(ProjectViewModelContext *parent);
        ~LabelSelectionController() override;

        QObjectList getSelectedItems() const override;
        QObjectList getItemsBetween(QObject *startItem, QObject *endItem) const override;
        void select(QObject *item, SelectionCommand command) override;
        QObject *currentItem() const override;
        bool editScopeFocused() const override;

    private:
        ProjectViewModelContext *q;
        dspx::LabelSequence *labelSequence;
        dspx::SelectionModel *selectionModel;
        dspx::LabelSelectionModel *labelSelectionModel;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_LABELSELECTIONCONTROLLER_P_H
