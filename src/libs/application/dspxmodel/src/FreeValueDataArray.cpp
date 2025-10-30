#include "FreeValueDataArray.h"

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/DataArrayData_p.h>

namespace dspx {

    class FreeValueDataArrayPrivate : public DataArrayData<FreeValueDataArray, int> {
        Q_DECLARE_PUBLIC(FreeValueDataArray)
    };

    FreeValueDataArray::FreeValueDataArray(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new FreeValueDataArrayPrivate) {
        Q_D(FreeValueDataArray);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ED_ParamCurveFreeValues);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    FreeValueDataArray::~FreeValueDataArray() = default;

    int FreeValueDataArray::size() const {
        Q_D(const FreeValueDataArray);
        return d->size();
    }

    bool FreeValueDataArray::splice(int index, int length, const QList<int> &values) {
        Q_D(FreeValueDataArray);
        QVariantList a;
        a.resize(values.size());
        std::ranges::transform(values, a.begin(), [](int v) { return QVariant(v); });
        return d->pModel->strategy->spliceDataArray(handle(), index, length, a);
    }

    QList<int> FreeValueDataArray::slice(int index, int length) const {
        Q_D(const FreeValueDataArray);
        if (index < 0 || index >= size())
            return {};
        return d->data.mid(index, length);
    }

    bool FreeValueDataArray::rotate(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(FreeValueDataArray);
        return d->pModel->strategy->rotateDataArray(handle(), leftIndex, middleIndex, rightIndex);
    }

    QList<int> FreeValueDataArray::toQDspx() const {
        Q_D(const FreeValueDataArray);
        return d->data;
    }

    void FreeValueDataArray::fromQDspx(const QList<int> &values) {
        splice(0, size(), values);
    }

    void FreeValueDataArray::handleSpliceDataArray(int index, int length, const QVariantList &values) {
        Q_D(FreeValueDataArray);
        QList<int> a;
        a.resize(values.size());
        std::ranges::transform(values, a.begin(), [](const QVariant &v) { return v.toInt(); });
        d->handleSpliceDataArray(index, length, a);
    }

    void FreeValueDataArray::handleRotateDataArray(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(FreeValueDataArray);
        d->handleRotateDataArray(leftIndex, middleIndex, rightIndex);
    }

}

#include "moc_FreeValueDataArray.cpp"