#ifndef DIFFSCOPE_VISUALEDITOR_CLIPSELECTIONCONTROLLER_P_H
#define DIFFSCOPE_VISUALEDITOR_CLIPSELECTIONCONTROLLER_P_H

#include <ScopicFlowCore/SelectionController.h>

namespace dspx {
    class SelectionModel;
    class Clip;
    class ClipSelectionModel;
    class Track;
    class TrackList;
}

namespace VisualEditor {

    class ProjectViewModelContext;

    class ClipSelectionController : public sflow::SelectionController {
        Q_OBJECT
    public:
        explicit ClipSelectionController(ProjectViewModelContext *parent);
        ~ClipSelectionController() override;

        QObjectList getSelectedItems() const override;
        QObjectList getItemsBetween(QObject *startItem, QObject *endItem) const override;
        void select(QObject *item, SelectionCommand command) override;
        QObject *currentItem() const override;
        bool editScopeFocused() const override;

    private:
        ProjectViewModelContext *q;
        dspx::SelectionModel *selectionModel;
        dspx::ClipSelectionModel *clipSelectionModel;
        dspx::TrackList *trackList;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_CLIPSELECTIONCONTROLLER_P_H
