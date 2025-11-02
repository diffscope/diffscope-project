#ifndef DIFFSCOPE_DSPX_MODEL_DATAARRAYDATA_P_H
#define DIFFSCOPE_DSPX_MODEL_DATAARRAYDATA_P_H

#include <algorithm>

#include <QVariant>
#include <QJSValue>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/SpliceHelper_p.h>

namespace dspx {

    class DataArrayJSIterable {
    public:
        static QJSValue create(QObject *o);
    };

    template <class DataArrayType, class DataType>
    class DataArrayData {
    public:
        DataArrayType *q_ptr;
        ModelPrivate *pModel;
        QList<DataType> data;
        QJSValue iterable_;

        int size() const {
            return data.size();
        }

        void handleSpliceDataArray(int index, int length, const QList<DataType> &values) {
            auto q = q_ptr;
            Q_EMIT q->aboutToSplice(index, length, values);
            SpliceHelper::splice(data, data.begin() + index, data.begin() + index + length, values.begin(), values.end());
            Q_EMIT q->spliced(index, length, values);
            if (values.size() != length) {
                Q_EMIT q->sizeChanged(size());
            }
        }

        void handleRotateDataArray(int leftIndex, int middleIndex, int rightIndex) {
            auto q = q_ptr;
            Q_EMIT q->aboutToRotate(leftIndex, middleIndex, rightIndex);
            std::rotate(data.begin() + leftIndex, data.begin() + middleIndex, data.begin() + rightIndex);
            Q_EMIT q->rotated(leftIndex, middleIndex, rightIndex);
        }

        QJSValue iterable() {
            if (!iterable_.isUndefined()) {
                return iterable_;
            }
            auto q = q_ptr;
            iterable_ = DataArrayJSIterable::create(q);
            return iterable_;
        }
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_DATAARRAYDATA_P_H
