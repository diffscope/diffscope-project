#include "ListData_p.h"

#include <QJSEngine>

namespace dspx {

    QJSValue ListJSIterable::create(QObject *o) {
        auto engine = qjsEngine(o);
        if (!engine) {
            qFatal() << "iterable() can only be called from QML";
            return {};
        }
        auto listIterableConstructor = engine->importModule(":/qt/qml/DiffScope/DspxModel/qml/iterable.mjs").property("ListIterable");
        Q_ASSERT(listIterableConstructor.isCallable());
        auto iterable = listIterableConstructor.callAsConstructor({engine->fromVariant<QJSValue>(QVariant::fromValue(o))});
        if (iterable.isError()) {
            qFatal() << iterable.property("message").toString();
            return {};
        }
        return iterable;
    }

}
