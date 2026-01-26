#ifndef DIFFSCOPE_DSPX_MODEL_NOTE_P_H
#define DIFFSCOPE_DSPX_MODEL_NOTE_P_H

#include <dspxmodel/Note.h>

namespace dspx {

    class NotePrivate {
        Q_DECLARE_PUBLIC(Note)
    public:
        Note *q_ptr;
        ModelPrivate *pModel;

        int centShift;
        int keyNum;
        QString language;
        int length;
        QString lyric;
        PhonemeInfo *phonemes;
        int pos;
        Pronunciation *pronunciation;
        Vibrato *vibrato;
        Workspace *workspace;
        NoteSequence *noteSequence{};
        bool overlapped{};



        static void setOverlapped(Note *item, bool overlapped);
        static void setNoteSequence(Note *item, NoteSequence *noteSequence);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTE_P_H