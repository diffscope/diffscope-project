#ifndef DIFFSCOPE_DSPX_MODEL_PHONEMESEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_PHONEMESEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>
#include <dspxmodel/rangehelpers.h>

namespace QDspx {
    struct Phoneme;
}

namespace dspx {

    class Phoneme;
    class PhonemeInfo;

    class PhonemeSequencePrivate;

    class DSPX_MODEL_EXPORT PhonemeSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PhonemeSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Phoneme *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Phoneme *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PROPERTY(PhonemeInfo *phonemeInfo READ phonemeInfo CONSTANT)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)

    public:
        ~PhonemeSequence() override;

        int size() const;
        Phoneme *firstItem() const;
        Phoneme *lastItem() const;
        Q_INVOKABLE Phoneme *previousItem(Phoneme *item) const;
        Q_INVOKABLE Phoneme *nextItem(Phoneme *item) const;
        Q_INVOKABLE QList<Phoneme *> slice(int position, int length) const;
        Q_INVOKABLE bool contains(Phoneme *item) const;

        Q_INVOKABLE bool insertItem(Phoneme *item);
        Q_INVOKABLE bool removeItem(Phoneme *item);

        QList<QDspx::Phoneme> toQDspx() const;
        void fromQDspx(const QList<QDspx::Phoneme> &phonemeList);

        PhonemeInfo *phonemeInfo() const;

        auto asRange() const {
            return impl::SequenceRange(this);
        }

    Q_SIGNALS:
        void itemAboutToInsert(Phoneme *item);
        void itemInserted(Phoneme *item);
        void itemAboutToRemove(Phoneme *item);
        void itemRemoved(Phoneme *item);
        void sizeChanged(int size);
        void firstItemChanged(Phoneme *item);
        void lastItemChanged(Phoneme *item);

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        explicit PhonemeSequence(PhonemeInfo *phonemeInfo, Handle handle, Model *model);
        QScopedPointer<PhonemeSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEMESEQUENCE_H
