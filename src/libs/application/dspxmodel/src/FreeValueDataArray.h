#ifndef DIFFSCOPE_DSPX_MODEL_FREEVALUEDATAARRAY_H
#define DIFFSCOPE_DSPX_MODEL_FREEVALUEDATAARRAY_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace dspx {

    class FreeValueDataArrayPrivate;

    class DSPX_MODEL_EXPORT FreeValueDataArray : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(FreeValueDataArray)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)

    public:
        ~FreeValueDataArray() override;

        int size() const;
        Q_INVOKABLE bool splice(int index, int length, const QList<int> &values);
        Q_INVOKABLE QList<int> slice(int index, int length) const;
        Q_INVOKABLE bool rotate(int leftIndex, int middleIndex, int rightIndex);

        QList<int> toQDspx() const;
        void fromQDspx(const QList<int> &values);

    Q_SIGNALS:
        void sizeChanged(int size);
        void aboutToSplice(int index, int length, const QList<int> &values);
        void spliced(int index, int length, const QList<int> &values);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);

    protected:
        void handleSpliceDataArray(int index, int length, const QVariantList &values) override;
        void handleRotateDataArray(int leftIndex, int middleIndex, int rightIndex) override;

    private:
        friend class ModelPrivate;
        explicit FreeValueDataArray(Handle handle, Model *model);
        QScopedPointer<FreeValueDataArrayPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_FREEVALUEDATAARRAY_H