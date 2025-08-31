#ifndef DIFFSCOPE_COREPLUGIN_PROJECTTIMELINE_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTTIMELINE_P_H

#include <coreplugin/projecttimeline.h>

namespace Core {

    class ProjectTimelinePrivate {
        Q_DECLARE_PUBLIC(ProjectTimeline)
    public:
        ProjectTimeline *q_ptr;
        SVS::MusicTimeline *musicTimeline{};
        int position{};
        int lastPosition{};
        int rangeHint{1};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTTIMELINE_P_H
