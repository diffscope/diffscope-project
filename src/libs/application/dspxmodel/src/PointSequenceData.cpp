#include "PointSequenceData_p.h"

#include <QJSEngine>

namespace dspx {

    QJSValue PointSequenceJSIterable::create(QObject *o) {
        auto engine = qjsEngine(o);
        if (!engine) {
            qFatal() << "iterable() can only be called from QML";
            return {};
        }
        auto sequenceIterableConstructor = engine->importModule(":/qt/qml/DiffScope/DspxModel/qml/iterable.mjs").property("SequenceIterable");
        Q_ASSERT(sequenceIterableConstructor.isCallable());
        auto iterable = sequenceIterableConstructor.callAsConstructor({engine->fromVariant<QJSValue>(QVariant::fromValue(o))});
        if (iterable.isError()) {
            qFatal() << iterable.property("message").toString();
            return {};
        }
        return iterable;
    }

}
