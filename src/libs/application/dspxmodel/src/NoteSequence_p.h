#ifndef DIFFSCOPE_DSPX_MODEL_NOTESEQUENCE_P_H
#define DIFFSCOPE_DSPX_MODEL_NOTESEQUENCE_P_H

#include <dspxmodel/NoteSequence.h>

#include <dspxmodel/private/RangeSequenceData_p.h>
#include <dspxmodel/private/Note_p.h>

namespace dspx {

    class NoteSequencePrivate : public RangeSequenceData<NoteSequence, Note, &Note::pos, &Note::posChanged, &Note::length, &Note::lengthChanged, &NotePrivate::setOverlapped, &NotePrivate::setNoteSequence> {
        Q_DECLARE_PUBLIC(NoteSequence)
    public:
        SingingClip *singingClip{};
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTESEQUENCE_P_H