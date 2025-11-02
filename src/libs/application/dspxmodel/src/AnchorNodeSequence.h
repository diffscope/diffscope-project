#ifndef DIFFSCOPE_DSPX_MODEL_ANCHORNODESEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_ANCHORNODESEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct AnchorNode;
}

namespace dspx {

    class AnchorNode;

    class AnchorNodeSequencePrivate;

    class DSPX_MODEL_EXPORT AnchorNodeSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AnchorNodeSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(AnchorNode *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(AnchorNode *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)
    public:
        ~AnchorNodeSequence() override;

        int size() const;
        AnchorNode *firstItem() const;
        AnchorNode *lastItem() const;
        Q_INVOKABLE AnchorNode *previousItem(AnchorNode *item) const;
        Q_INVOKABLE AnchorNode *nextItem(AnchorNode *item) const;
        Q_INVOKABLE QList<AnchorNode *> slice(int position, int length) const;
        Q_INVOKABLE bool contains(AnchorNode *item) const;

        Q_INVOKABLE bool insertItem(AnchorNode *item);
        Q_INVOKABLE bool removeItem(AnchorNode *item);

        QList<QDspx::AnchorNode> toQDspx() const;
        void fromQDspx(const QList<QDspx::AnchorNode> &nodes);

    Q_SIGNALS:
        void itemAboutToInsert(AnchorNode *item);
        void itemInserted(AnchorNode *item);
        void itemAboutToRemove(AnchorNode *item);
        void itemRemoved(AnchorNode *item);
        void sizeChanged(int size);
        void firstItemChanged(AnchorNode *item);
        void lastItemChanged(AnchorNode *item);

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        explicit AnchorNodeSequence(Handle handle, Model *model);
        QScopedPointer<AnchorNodeSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ANCHORNODESEQUENCE_H
