#ifndef DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_P_H
#define DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_P_H

#include <visualeditor/ArrangementPanelInterface.h>

namespace VisualEditor {

    class ArrangementPanelInterfacePrivate {
        Q_DECLARE_PUBLIC(ArrangementPanelInterface)
    public:
        ArrangementPanelInterface *q_ptr;

        Core::ProjectWindowInterface *windowHandle;
        sflow::TimeViewModel *timeViewModel;
        sflow::TimeLayoutViewModel *timeLayoutViewModel;
        sflow::TimelineInteractionController *timelineInteractionController;
        sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel;

        QQuickItem *arrangementView;

        ArrangementPanelInterface::Tool tool{ArrangementPanelInterface::PointerTool};

        void bindTimeViewModel() const;
        void bindTimeLayoutViewModel() const;
        void bindTimelineInteractionController() const;
        void bindScrollBehaviorViewModel() const;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_P_H