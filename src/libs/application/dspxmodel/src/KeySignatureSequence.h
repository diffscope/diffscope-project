#ifndef DIFFSCOPE_DSPX_MODEL_KEYSIGNATURESEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_KEYSIGNATURESEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>
#include <dspxmodel/rangehelpers.h>

namespace dspx {

    class KeySignature;

    class KeySignatureSequencePrivate;

    class DSPX_MODEL_EXPORT KeySignatureSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(KeySignatureSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(KeySignature *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(KeySignature *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)
    public:
        ~KeySignatureSequence() override;

        int size() const;
        KeySignature *firstItem() const;
        KeySignature *lastItem() const;
        Q_INVOKABLE KeySignature *previousItem(KeySignature *item) const;
        Q_INVOKABLE KeySignature *nextItem(KeySignature *item) const;
        Q_INVOKABLE QList<KeySignature *> slice(int position, int length) const;
        Q_INVOKABLE KeySignature *itemAt(int position) const;
        Q_INVOKABLE bool contains(KeySignature *item) const;

        Q_INVOKABLE bool insertItem(KeySignature *item);
        Q_INVOKABLE bool removeItem(KeySignature *item);

        QJsonArray toQDspx() const;
        void fromQDspx(const QJsonArray &keySignatures);

        auto asRange() const {
            return impl::SequenceRange(this);
        }

    Q_SIGNALS:
        void itemAboutToInsert(KeySignature *item);
        void itemInserted(KeySignature *item);
        void itemAboutToRemove(KeySignature *item);
        void itemRemoved(KeySignature *item);
        void sizeChanged(int size);
        void firstItemChanged(KeySignature *item);
        void lastItemChanged(KeySignature *item);

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        explicit KeySignatureSequence(Handle handle, Model *model);
        QScopedPointer<KeySignatureSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_KEYSIGNATURESEQUENCE_H
