#ifndef DIFFSCOPE_VISUALEDITOR_TRACKSELECTIONCONTROLLER_P_H
#define DIFFSCOPE_VISUALEDITOR_TRACKSELECTIONCONTROLLER_P_H

#include <ScopicFlowCore/SelectionController.h>

namespace dspx {
    class SelectionModel;
    class Track;
    class TrackSelectionModel;
    class TrackList;
}

namespace VisualEditor {

    class ProjectViewModelContext;

    class TrackSelectionController : public sflow::SelectionController {
        Q_OBJECT
    public:
        explicit TrackSelectionController(ProjectViewModelContext *parent);
        ~TrackSelectionController() override;

        QObjectList getSelectedItems() const override;
        QObjectList getItemsBetween(QObject *startItem, QObject *endItem) const override;
        void select(QObject *item, SelectionCommand command) override;
        QObject *currentItem() const override;
        bool editScopeFocused() const override;

    private:
        ProjectViewModelContext *q;
        dspx::TrackList *trackList;
        dspx::SelectionModel *selectionModel;
        dspx::TrackSelectionModel *trackSelectionModel;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_TRACKSELECTIONCONTROLLER_P_H
