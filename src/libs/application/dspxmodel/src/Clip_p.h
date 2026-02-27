#ifndef DIFFSCOPE_DSPX_MODEL_CLIP_P_H
#define DIFFSCOPE_DSPX_MODEL_CLIP_P_H

#include <dspxmodel/Clip.h>

namespace dspx {

    class ClipPrivate {
        Q_DECLARE_PUBLIC(Clip)
    public:
        Clip *q_ptr;
        ModelPrivate *pModel;
        QString name;
        BusControl *control;
        ClipTime *time;
        Clip::ClipType type;
        ClipSequence *clipSequence{};
        Clip *previousItem{};
        Clip *nextItem{};
        Workspace *workspace;
        bool overlapped{};

        static void setOverlapped(Clip *item, bool overlapped);
        static void setClipSequence(Clip *item, ClipSequence *clipSequence);
        static void setPreviousItem(Clip *item, Clip *previousItem);
        static void setNextItem(Clip *item, Clip *nextItem);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CLIP_P_H
