#ifndef DIFFSCOPE_DSPX_MODEL_LABELSEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_LABELSEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace dspx {

    class Label;

    class LabelSequencePrivate;

    class LabelSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(LabelSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Label *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Label *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue items READ iterable CONSTANT)
    public:
        ~LabelSequence() override;

        int size() const;
        Label *firstItem() const;
        Label *lastItem() const;
        Q_INVOKABLE Label *previousItem(Label *item) const;
        Q_INVOKABLE Label *nextItem(Label *item) const;
        Q_INVOKABLE QList<Label *> slice(int position, int length) const;
        Q_INVOKABLE bool contains(Label *item) const;

        Q_INVOKABLE void insertItem(Label *item);
        Q_INVOKABLE void removeItem(Label *item);

    Q_SIGNALS:
        void itemAboutToInsert(Label *item);
        void itemInserted(Label *item);
        void itemAboutToRemove(Label *item);
        void itemRemoved(Label *item);
        void sizeChanged(int size);
        void firstItemChanged(Label *item);
        void lastItemChanged(Label *item);

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        explicit LabelSequence(Handle handle, Model *model);
        QScopedPointer<LabelSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LABELSEQUENCE_H
