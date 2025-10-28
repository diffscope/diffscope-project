#ifndef DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H

#include <coreplugin/projectviewmodelcontext.h>

namespace Core {

    class ProjectViewModelContextPrivate {
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        sflow::PlaybackViewModel *playbackViewModel;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H
