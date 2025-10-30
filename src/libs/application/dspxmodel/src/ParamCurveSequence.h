#ifndef DIFFSCOPE_DSPX_MODEL_PARAMCURVESEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_PARAMCURVESEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct ParamCurve;
    using ParamCurveRef = QSharedPointer<ParamCurve>;
}

namespace dspx {

    class ParamCurve;

    class ParamCurveSequencePrivate;

    class DSPX_MODEL_EXPORT ParamCurveSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ParamCurveSequence)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(ParamCurve *firstItem READ firstItem NOTIFY firstItemChanged)
        Q_PROPERTY(ParamCurve *lastItem READ lastItem NOTIFY lastItemChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)
    public:
        ~ParamCurveSequence() override;

        int size() const;
        ParamCurve *firstItem() const;
        ParamCurve *lastItem() const;
        Q_INVOKABLE ParamCurve *previousItem(ParamCurve *item) const;
        Q_INVOKABLE ParamCurve *nextItem(ParamCurve *item) const;
        Q_INVOKABLE QList<ParamCurve *> slice(int position, int length) const;
        Q_INVOKABLE bool contains(ParamCurve *item) const;

        Q_INVOKABLE bool insertItem(ParamCurve *item);
        Q_INVOKABLE bool removeItem(ParamCurve *item);

        QList<QDspx::ParamCurveRef> toQDspx() const;
        void fromQDspx(const QList<QDspx::ParamCurveRef> &curves);

    Q_SIGNALS:
        void itemAboutToInsert(ParamCurve *item);
        void itemInserted(ParamCurve *item);
        void itemAboutToRemove(ParamCurve *item);
        void itemRemoved(ParamCurve *item);
        void sizeChanged(int size);
        void firstItemChanged(ParamCurve *item);
        void lastItemChanged(ParamCurve *item);

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        explicit ParamCurveSequence(Handle handle, Model *model);
        QScopedPointer<ParamCurveSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PARAMCURVESEQUENCE_H