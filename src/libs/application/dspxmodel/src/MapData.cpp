#include "MapData_p.h"

#include <QJSEngine>

namespace dspx {

    QJSValue MapJSIterable::create(QObject *o) {
        auto engine = qjsEngine(o);
        if (!engine) {
            qFatal() << "iterable() can only be called from QML";
            return {};
        }
        auto mapIterableConstructor = engine->importModule(":/qt/qml/DiffScope/DspxModel/qml/iterable.mjs").property("MapIterable");
        Q_ASSERT(mapIterableConstructor.isCallable());
        auto iterable = mapIterableConstructor.callAsConstructor({engine->fromVariant<QJSValue>(QVariant::fromValue(o))});
        if (iterable.isError()) {
            qFatal() << iterable.property("message").toString();
            return {};
        }
        return iterable;
    }

}
