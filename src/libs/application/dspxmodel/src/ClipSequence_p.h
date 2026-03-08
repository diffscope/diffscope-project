#ifndef DIFFSCOPE_DSPX_MODEL_CLIPSEQUENCE_P_H
#define DIFFSCOPE_DSPX_MODEL_CLIPSEQUENCE_P_H

#include <dspxmodel/ClipSequence.h>

#include <dspxmodel/private/RangeSequenceData_p.h>
#include <dspxmodel/private/Clip_p.h>

namespace dspx {

    class ClipSequencePrivate : public RangeSequenceData<ClipSequence, Clip, &Clip::position, &Clip::positionChanged, &Clip::length, &Clip::lengthChanged, &ClipPrivate::setOverlapped, &ClipPrivate::setClipSequence, &ClipPrivate::setPreviousItem, &ClipPrivate::setNextItem> {
        Q_DECLARE_PUBLIC(ClipSequence)
    public:
        Track *track{};

        void handleMoveFromAnotherSequenceContainer(Handle entity, Handle otherSequenceContainerEntity);
        void handleMoveToAnotherSequenceContainer(Handle entity, Handle otherSequenceContainerEntity);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CLIPSEQUENCE_P_H
