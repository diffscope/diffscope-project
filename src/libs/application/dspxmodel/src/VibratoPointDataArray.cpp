#include "VibratoPointDataArray.h"

#include <QPointF>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/DataArrayData_p.h>

namespace dspx {

    class VibratoPointDataArrayPrivate : public DataArrayData<VibratoPointDataArray, QPointF> {
        Q_DECLARE_PUBLIC(VibratoPointDataArray)
    };

    VibratoPointDataArray::VibratoPointDataArray(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new VibratoPointDataArrayPrivate) {
        Q_D(VibratoPointDataArray);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ED_VibratoPoints);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    VibratoPointDataArray::~VibratoPointDataArray() = default;

    int VibratoPointDataArray::size() const {
        Q_D(const VibratoPointDataArray);
        return d->size();
    }

    bool VibratoPointDataArray::splice(int index, int length, const QList<QPointF> &values) {
        Q_D(VibratoPointDataArray);
        QVariantList a;
        a.resize(values.size());
        std::ranges::transform(values, a.begin(), [](const QPointF &p) { return QVariant(p); });
        return d->pModel->strategy->spliceDataArray(handle(), index, length, a);
    }

    QList<QPointF> VibratoPointDataArray::slice(int index, int length) const {
        Q_D(const VibratoPointDataArray);
        if (index < 0 || index >= size())
            return {};
        return d->data.mid(index, length);
    }
    bool VibratoPointDataArray::rotate(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(VibratoPointDataArray);
        return d->pModel->strategy->rotateDataArray(handle(), leftIndex, middleIndex, rightIndex);
    }

    QList<QDspx::Point<double>> VibratoPointDataArray::toQDspx() const {
        Q_D(const VibratoPointDataArray);
        QList<QDspx::Point<double>> a;
        a.resize(d->data.size());
        std::ranges::transform(d->data, a.begin(), [](const QPointF &p) -> QDspx::Point<double> { return {p.x(), p.y()}; });
        return a;
    }

    void VibratoPointDataArray::fromQDspx(const QList<QDspx::Point<double>> &vibratoPoints) {
        QList<QPointF> a;
        a.resize(vibratoPoints.size());
        std::ranges::transform(vibratoPoints, a.begin(), [](const QDspx::Point<double> &p) -> QPointF { return {p.x, p.y}; });
        splice(0, size(), a);
    }

    void VibratoPointDataArray::handleSpliceDataArray(int index, int length, const QVariantList &values) {
        Q_D(VibratoPointDataArray);
        QList<QPointF> a;
        a.resize(values.size());
        std::ranges::transform(values, a.begin(), [](const QVariant &v) { return v.toPointF(); });
        d->handleSpliceDataArray(index, length, a);
    }
    void VibratoPointDataArray::handleRotateDataArray(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(VibratoPointDataArray);
        d->handleRotateDataArray(leftIndex, middleIndex, rightIndex);
    }
}

#include "moc_VibratoPointDataArray.cpp"