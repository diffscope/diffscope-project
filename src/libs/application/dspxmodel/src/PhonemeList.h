#ifndef DIFFSCOPE_DSPX_MODEL_PHONEMELIST_H
#define DIFFSCOPE_DSPX_MODEL_PHONEMELIST_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Phoneme;
}

namespace dspx {

    class Phoneme;

    class PhonemeListPrivate;

    class DSPX_MODEL_EXPORT PhonemeList : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PhonemeList)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QList<Phoneme *> items READ items NOTIFY itemsChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)

    public:
        ~PhonemeList() override;

        int size() const;
        QList<Phoneme *> items() const;
        Q_INVOKABLE bool insertItem(int index, Phoneme *item);
        Q_INVOKABLE Phoneme *removeItem(int index);
        Q_INVOKABLE Phoneme *item(int index) const;
        Q_INVOKABLE bool rotate(int leftIndex, int middleIndex, int rightIndex);

        QList<QDspx::Phoneme> toQDspx() const;
        void fromQDspx(const QList<QDspx::Phoneme> &phonemeList);

    Q_SIGNALS:
        void itemAboutToInsert(int index, Phoneme *item);
        void itemInserted(int index, Phoneme *item);
        void itemAboutToRemove(int index, Phoneme *item);
        void itemRemoved(int index, Phoneme *item);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);
        void sizeChanged(int size);
        void itemsChanged();

    protected:
        void handleInsertIntoListContainer(Handle entity, int index) override;
        void handleTakeFromListContainer(Handle takenEntity, int index) override;
        void handleRotateListContainer(int leftIndex, int middleIndex, int rightIndex) override;

    private:
        friend class ModelPrivate;
        explicit PhonemeList(Handle handle, Model *model);
        QScopedPointer<PhonemeListPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEMELIST_H
