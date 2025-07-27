#include "USDef.h"

#include <QObject>
#include <qqmlintegration.h>

namespace UIShell {

    class USDefForeign : public QObject {
        Q_OBJECT
        QML_NAMED_ELEMENT(USDef)
        QML_FOREIGN_NAMESPACE(UIShell::USDef)
        QML_SINGLETON
    };

}

#include "USDef.moc"