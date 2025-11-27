#ifndef DIFFSCOPE_DSPX_MODEL_TRACK_P_H
#define DIFFSCOPE_DSPX_MODEL_TRACK_P_H

#include <dspxmodel/Track.h>

namespace dspx {

    class TrackPrivate {
        Q_DECLARE_PUBLIC(Track)
    public:
        Track *q_ptr;
        ModelPrivate *pModel;
        ClipSequence *clips;
        QString name;
        TrackControl *control;
        Workspace *workspace;
        TrackList *trackList;

        static void setTrackList(Track *item, TrackList *trackList);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TRACK_P_H
