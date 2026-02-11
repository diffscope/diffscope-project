#ifndef DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_P_H
#define DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_P_H

#include <visualeditor/PianoRollPanelInterface.h>
#include <visualeditor/PositionAlignmentManipulator.h>

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
        sflow::ClavierViewModel *clavierViewModel;
        sflow::ClavierInteractionController *clavierInteractionController;

        PositionAlignmentManipulator *positionAlignmentManipulator;
        AutoPageScrollingManipulator *autoPageScrollingManipulator;

        QQuickItem *pianoRollView;

        PianoRollPanelInterface::Tool tool{PianoRollPanelInterface::PointerTool};
        bool isSnapTemporarilyDisabled{false};
        bool isMouseTrackingDisabled{false};

        mutable PositionAlignmentManipulator::Duration previousDuration{};

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
