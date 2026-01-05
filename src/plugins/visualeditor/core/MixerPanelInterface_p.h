#ifndef DIFFSCOPE_VISUALEDITOR_MIXERPANELINTERFACE_P_H
#define DIFFSCOPE_VISUALEDITOR_MIXERPANELINTERFACE_P_H

#include <visualeditor/MixerPanelInterface.h>

namespace VisualEditor {

    class MixerPanelInterfacePrivate {
        Q_DECLARE_PUBLIC(MixerPanelInterface)
    public:
        MixerPanelInterface *q_ptr;

        Core::ProjectWindowInterface *windowHandle;

        Internal::MixerAddOn *addon;

        sflow::TrackListLayoutViewModel *trackListLayoutViewModel;
        sflow::TrackListLayoutViewModel *masterTrackListLayoutViewModel;
        sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel;
        sflow::TrackListInteractionController *trackListInteractionController;
        sflow::TrackListInteractionController *masterTrackListInteractionController;

        QQuickItem *mixerView;

        MixerPanelInterface::Tool tool{MixerPanelInterface::PointerTool};

        void bindScrollBehaviorViewModel() const;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_MIXERPANELINTERFACE_P_H
