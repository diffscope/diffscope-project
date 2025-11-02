#ifndef DIFFSCOPE_DSPX_MODEL_NOTESEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_NOTESEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Note;
}

namespace dspx {

    class Note;
    class SingingClip;

    class NoteSequencePrivate;

    class DSPX_MODEL_EXPORT NoteSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(NoteSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Note *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Note *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(SingingClip *singingClip READ singingClip CONSTANT)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)

    public:
        ~NoteSequence() override;

        int size() const;
        Note *firstItem() const;
        Note *lastItem() const;
        Q_INVOKABLE Note *previousItem(Note *item) const;
        Q_INVOKABLE Note *nextItem(Note *item) const;
        Q_INVOKABLE QList<Note *> slice(int position, int length) const;
        Q_INVOKABLE bool contains(Note *item) const;

        Q_INVOKABLE bool insertItem(Note *item);
        Q_INVOKABLE bool removeItem(Note *item);

        QList<QDspx::Note> toQDspx() const;
        void fromQDspx(const QList<QDspx::Note> &notes);

        SingingClip *singingClip() const;

    Q_SIGNALS:
        void itemAboutToInsert(Note *item);
        void itemInserted(Note *item);
        void itemAboutToRemove(Note *item);
        void itemRemoved(Note *item);
        void sizeChanged(int size);
        void firstItemChanged(Note *item);
        void lastItemChanged(Note *item);

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        friend class SingingClip;
        explicit NoteSequence(Handle handle, Model *model);
        void setSingingClip(SingingClip *singingClip);
        QScopedPointer<NoteSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTESEQUENCE_H