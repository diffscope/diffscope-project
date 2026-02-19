#ifndef DIFFSCOPE_VISUALEDITOR_NOTESELECTIONCONTROLLER_P_H
#define DIFFSCOPE_VISUALEDITOR_NOTESELECTIONCONTROLLER_P_H

#include <ScopicFlowCore/SelectionController.h>

namespace dspx {
    class SelectionModel;
    class Note;
    class NoteSelectionModel;
    class NoteSequence;
}

namespace VisualEditor {

    class ProjectViewModelContext;

    class NoteSelectionController : public sflow::SelectionController {
        Q_OBJECT
    public:
        explicit NoteSelectionController(ProjectViewModelContext *parent);
        ~NoteSelectionController() override;

        QObjectList getSelectedItems() const override;
        QObjectList getItemsBetween(QObject *startItem, QObject *endItem) const override;
        void select(QObject *item, SelectionCommand command) override;
        QObject *currentItem() const override;
        bool editScopeFocused() const override;

    private:
        ProjectViewModelContext *q;
        dspx::SelectionModel *selectionModel;
        dspx::NoteSelectionModel *noteSelectionModel;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_NOTESELECTIONCONTROLLER_P_H
