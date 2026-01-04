#ifndef DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_P_H
#define DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_P_H

#include <visualeditor/ArrangementPanelInterface.h>
#include <visualeditor/PositionAlignmentManipulator.h>

namespace VisualEditor {

    class ArrangementPanelInterfacePrivate {
        Q_DECLARE_PUBLIC(ArrangementPanelInterface)
    public:
        ArrangementPanelInterface *q_ptr;

        Core::ProjectWindowInterface *windowHandle;

        Internal::ArrangementAddOn *addon;

        sflow::TimeViewModel *timeViewModel;
        sflow::TimeLayoutViewModel *timeLayoutViewModel;
        sflow::TrackListLayoutViewModel *trackListLayoutViewModel;
        sflow::TimelineInteractionController *timelineInteractionController;
        sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel;
        sflow::LabelSequenceInteractionController *labelSequenceInteractionControllerOfTempo;
        sflow::LabelSequenceInteractionController *labelSequenceInteractionControllerOfLabel;
        sflow::TrackListInteractionController *trackListInteractionController;

        PositionAlignmentManipulator *positionAlignmentManipulator;
        AutoPageScrollingManipulator *autoPageScrollingManipulator;

        QQuickItem *arrangementView;

        ArrangementPanelInterface::Tool tool{ArrangementPanelInterface::PointerTool};
        bool isSnapTemporarilyDisabled{false};
        bool isMouseTrackingDisabled{false};

        mutable PositionAlignmentManipulator::Duration previousDuration{};

        void bindTimeViewModel() const;
        void bindTimeLayoutViewModel() const;
        void bindTimelineInteractionController() const;
        void bindScrollBehaviorViewModel() const;
        void bindPositionAlignmentManipulator() const;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_P_H