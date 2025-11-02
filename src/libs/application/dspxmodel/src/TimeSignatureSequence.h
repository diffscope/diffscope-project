#ifndef DIFFSCOPE_DSPX_MODEL_TIMESIGNATURESEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_TIMESIGNATURESEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct TimeSignature;
}

namespace dspx {

    class TimeSignature;

    class TimeSignatureSequencePrivate;

    class DSPX_MODEL_EXPORT TimeSignatureSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(TimeSignatureSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(TimeSignature *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(TimeSignature *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)
    public:
        ~TimeSignatureSequence() override;

        int size() const;
        TimeSignature *firstItem() const;
        TimeSignature *lastItem() const;
        Q_INVOKABLE TimeSignature *previousItem(TimeSignature *item) const;
        Q_INVOKABLE TimeSignature *nextItem(TimeSignature *item) const;
        Q_INVOKABLE QList<TimeSignature *> slice(int position, int length) const;
        Q_INVOKABLE bool contains(TimeSignature *item) const;

        Q_INVOKABLE bool insertItem(TimeSignature *item);
        Q_INVOKABLE bool removeItem(TimeSignature *item);

        QList<QDspx::TimeSignature> toQDspx() const;
        void fromQDspx(const QList<QDspx::TimeSignature> &timeSignatures);

    Q_SIGNALS:
        void itemAboutToInsert(TimeSignature *item);
        void itemInserted(TimeSignature *item);
        void itemAboutToRemove(TimeSignature *item);
        void itemRemoved(TimeSignature *item);
        void sizeChanged(int size);
        void firstItemChanged(TimeSignature *item);
        void lastItemChanged(TimeSignature *item);

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        explicit TimeSignatureSequence(Handle handle, Model *model);
        QScopedPointer<TimeSignatureSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TIMESIGNATURESEQUENCE_H