#ifndef VIBRATOPOINTDATAARRAY_H
#define VIBRATOPOINTDATAARRAY_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct ControlPoint;
}

namespace dspx {

    class VibratoPointDataArrayPrivate;

    class DSPX_MODEL_EXPORT VibratoPointDataArray : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(VibratoPointDataArray)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)

    public:
        ~VibratoPointDataArray() override;

        int size() const;
        Q_INVOKABLE bool splice(int index, int length, const QList<QPointF> &values);
        Q_INVOKABLE QList<QPointF> slice(int index, int length) const;
        Q_INVOKABLE bool rotate(int leftIndex, int middleIndex, int rightIndex);

        QList<QDspx::ControlPoint> toQDspx() const;
        void fromQDspx(const QList<QDspx::ControlPoint> &vibratoPoints);

    Q_SIGNALS:
        void sizeChanged(int size);
        void aboutToSplice(int index, int length, const QList<QPointF> &values);
        void spliced(int index, int length, const QList<QPointF> &values);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);

    protected:
        void handleSpliceDataArray(int index, int length, const QVariantList &values) override;
        void handleRotateDataArray(int leftIndex, int middleIndex, int rightIndex) override;

    private:
        friend class ModelPrivate;
        explicit VibratoPointDataArray(Handle handle, Model *model);
        QScopedPointer<VibratoPointDataArrayPrivate> d_ptr;

    };

}

#endif //VIBRATOPOINTDATAARRAY_H
