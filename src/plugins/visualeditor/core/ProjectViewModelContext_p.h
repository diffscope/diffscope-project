#ifndef DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H

#include <memory>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    class PlaybackViewModelContextData;
    class TempoViewModelContextData;

    class ProjectViewModelContextPrivate {
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::ProjectWindowInterface *windowHandle;

        std::unique_ptr<PlaybackViewModelContextData> playbackData;
        std::unique_ptr<TempoViewModelContextData> tempoData;
    };
}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H
