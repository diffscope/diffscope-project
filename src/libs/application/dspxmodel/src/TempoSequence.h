#ifndef DIFFSCOPE_DSPX_MODEL_TEMPOSEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_TEMPOSEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Tempo;
}

namespace dspx {

    class Tempo;

    class TempoSequencePrivate;

    class DSPX_MODEL_EXPORT TempoSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(TempoSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(Tempo *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(Tempo *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue items READ iterable CONSTANT)
    public:
        ~TempoSequence() override;

        int size() const;
        Tempo *firstItem() const;
        Tempo *lastItem() const;
        Q_INVOKABLE Tempo *previousItem(Tempo *item) const;
        Q_INVOKABLE Tempo *nextItem(Tempo *item) const;
        Q_INVOKABLE QList<Tempo *> slice(int position, int length) const;
        Q_INVOKABLE bool contains(Tempo *item) const;

        Q_INVOKABLE bool insertItem(Tempo *item);
        Q_INVOKABLE bool removeItem(Tempo *item);

        QList<QDspx::Tempo> toQDspx() const;
        void fromQDspx(const QList<QDspx::Tempo> &tempos);

    Q_SIGNALS:
        void itemAboutToInsert(Tempo *item);
        void itemInserted(Tempo *item);
        void itemAboutToRemove(Tempo *item);
        void itemRemoved(Tempo *item);
        void sizeChanged(int size);
        void firstItemChanged(Tempo *item);
        void lastItemChanged(Tempo *item);

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        explicit TempoSequence(Handle handle, Model *model);
        QScopedPointer<TempoSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPOSEQUENCE_H