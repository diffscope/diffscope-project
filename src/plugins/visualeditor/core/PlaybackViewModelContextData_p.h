#ifndef DIFFSCOPE_VISUALEDITOR_PLAYBACKVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_PLAYBACKVIEWMODELCONTEXTDATA_P_H

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    class PlaybackViewModelContextData {
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::ProjectWindowInterface *windowHandle;
        sflow::PlaybackViewModel *playbackViewModel;

        void init();
        void bindPlaybackViewModel();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_PLAYBACKVIEWMODELCONTEXTDATA_P_H
