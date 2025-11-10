#ifndef DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    class ProjectViewModelContextPrivate {
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::ProjectWindowInterface *windowHandle;
        sflow::PlaybackViewModel *playbackViewModel;

        void bindPlaybackViewModel() const;
    };
}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H
