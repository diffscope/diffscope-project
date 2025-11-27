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

        void setCentShiftUnchecked(int centShift_);
        void setCentShift(int centShift_);
        void setKeyNumUnchecked(int keyNum_);
        void setKeyNum(int keyNum_);
        void setLengthUnchecked(int length_);
        void setLength(int length_);
        void setPosUnchecked(int pos_);
        void setPos(int pos_);

        static void setOverlapped(Note *item, bool overlapped);
        static void setNoteSequence(Note *item, NoteSequence *noteSequence);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTE_P_H