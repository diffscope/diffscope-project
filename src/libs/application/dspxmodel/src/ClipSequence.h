#ifndef DIFFSCOPE_DSPX_MODEL_CLIPSEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_CLIPSEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Clip;
    using ClipRef = QSharedPointer<Clip>;
}

namespace dspx {

    class Clip;
    class Track;

    class ClipSequencePrivate;

    class DSPX_MODEL_EXPORT ClipSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ClipSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Clip *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Clip *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(Track *track READ track CONSTANT)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)
    public:
        ~ClipSequence() override;

        int size() const;
        Clip *firstItem() const;
        Clip *lastItem() const;
        Q_INVOKABLE Clip *previousItem(Clip *item) const;
        Q_INVOKABLE Clip *nextItem(Clip *item) const;
        Q_INVOKABLE QList<Clip *> slice(int position, int length) const;
        Q_INVOKABLE bool contains(Clip *item) const;

        Q_INVOKABLE bool insertItem(Clip *item);
        Q_INVOKABLE bool removeItem(Clip *item);

        QList<QDspx::ClipRef> toQDspx() const;
        void fromQDspx(const QList<QDspx::ClipRef> &clips);

        Track *track() const;

    Q_SIGNALS:
        void itemAboutToInsert(Clip *item);
        void itemInserted(Clip *item);
        void itemAboutToRemove(Clip *item);
        void itemRemoved(Clip *item);
        void sizeChanged(int size);
        void firstItemChanged(Clip *item);
        void lastItemChanged(Clip *item);

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        explicit ClipSequence(Track *track, Handle handle, Model *model);
        QScopedPointer<ClipSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CLIPSEQUENCE_H
