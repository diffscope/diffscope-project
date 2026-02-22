#ifndef DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_P_H
#define DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_P_H

#include <visualeditor/PianoRollPanelInterface.h>
#include <visualeditor/PositionAlignmentManipulator.h>

namespace dspx {
    class SelectionModel;
}

class QSortFilterProxyModel;

namespace VisualEditor::Internal {
    class SingingClipListModel;
    class TrackOverlaySelectorModel;
}

namespace VisualEditor {

    class PianoRollPanelInterfacePrivate {
        Q_DECLARE_PUBLIC(PianoRollPanelInterface)
    public:
        PianoRollPanelInterface *q_ptr;

        Core::ProjectWindowInterface *windowHandle;

        Internal::PianoRollAddOn *addon;

        sflow::TimeViewModel *timeViewModel;
        sflow::TimeLayoutViewModel *timeLayoutViewModel;
        sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel;
        sflow::TimelineInteractionController *timelineInteractionController;
        sflow::LabelSequenceInteractionController *labelSequenceInteractionControllerOfTempo;
        sflow::LabelSequenceInteractionController *labelSequenceInteractionControllerOfLabel;
        sflow::NoteEditLayerInteractionController *noteEditLayerInteractionController;
        sflow::ClavierViewModel *clavierViewModel;
        sflow::ClavierInteractionController *clavierInteractionController;

        PositionAlignmentManipulator *positionAlignmentManipulator;
        AutoPageScrollingManipulator *autoPageScrollingManipulator;

        QQuickItem *pianoRollView;
        Internal::TrackOverlaySelectorModel *trackOverlaySelectorModel;
        Internal::SingingClipListModel *singingClipListModel;
        QSortFilterProxyModel *editingClipSelectorModel;

        PianoRollPanelInterface::Tool tool{PianoRollPanelInterface::PointerTool};
        bool isSnapTemporarilyDisabled{false};
        bool isMouseTrackingDisabled{false};

        dspx::SelectionModel *editingClipSelectionModel;

        mutable PositionAlignmentManipulator::Duration previousDuration{};
        mutable int previousPositionAlignment{};

        void bindTimeViewModel() const;
        void bindTimeLayoutViewModel() const;
        void bindTimelineInteractionController() const;
        void bindScrollBehaviorViewModel() const;
        void bindPositionAlignmentManipulator() const;
        void bindControllersInteraction() const;
        void bindClavierInteractionController() const;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_P_H
