#ifndef DIFFSCOPE_DSPX_MODEL_NOTE_H
#define DIFFSCOPE_DSPX_MODEL_NOTE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Note;
}

namespace dspx {

    class PhonemeInfo;
    class Pronunciation;
    class Vibrato;
    class Workspace;
    class NoteSequence;

    class NoteSequencePrivate;

    class NotePrivate;

    class DSPX_MODEL_EXPORT Note : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Note)
        Q_PROPERTY(int centShift READ centShift WRITE setCentShift NOTIFY centShiftChanged)
        Q_PROPERTY(int keyNum READ keyNum WRITE setKeyNum NOTIFY keyNumChanged)
        Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
        Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
        Q_PROPERTY(QString lyric READ lyric WRITE setLyric NOTIFY lyricChanged)
        Q_PROPERTY(PhonemeInfo *phonemes READ phonemes CONSTANT)
        Q_PROPERTY(int pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(Pronunciation *pronunciation READ pronunciation CONSTANT)
        Q_PROPERTY(Vibrato *vibrato READ vibrato CONSTANT)
        Q_PROPERTY(Workspace *workspace READ workspace CONSTANT)
        Q_PROPERTY(NoteSequence *noteSequence READ noteSequence NOTIFY noteSequenceChanged)
        Q_PROPERTY(Note *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Note *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(bool overlapped READ isOverlapped NOTIFY overlappedChanged)

    public:
        ~Note() override;

        int centShift() const;
        void setCentShift(int centShift);

        int keyNum() const;
        void setKeyNum(int keyNum);

        QString language() const;
        void setLanguage(const QString &language);

        int length() const;
        void setLength(int length);

        QString lyric() const;
        void setLyric(const QString &lyric);

        PhonemeInfo *phonemes() const;

        int pos() const;
        void setPos(int pos);

        Pronunciation *pronunciation() const;

        Vibrato *vibrato() const;

        Workspace *workspace() const;

        NoteSequence *noteSequence() const;

        Note *previousItem() const;
        Note *nextItem() const;

        bool isOverlapped() const;

        QDspx::Note toQDspx() const;
        void fromQDspx(const QDspx::Note &note);

    Q_SIGNALS:
        void centShiftChanged(int centShift);
        void keyNumChanged(int keyNum);
        void languageChanged(const QString &language);
        void lengthChanged(int length);
        void lyricChanged(const QString &lyric);
        void posChanged(int pos);
        void noteSequenceChanged();
        void previousItemChanged();
        void nextItemChanged();
        void overlappedChanged(bool overlapped);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit Note(Handle handle, Model *model);
        QScopedPointer<NotePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTE_H
