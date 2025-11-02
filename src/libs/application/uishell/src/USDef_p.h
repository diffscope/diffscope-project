#ifndef UISHELL_USDEF_P_H
#define UISHELL_USDEF_P_H

#include <qqmlintegration.h>

#include <QObject>

#include <uishell/USDef.h>

namespace UIShell {

    class USDefForeign : public QObject {
        Q_OBJECT
        QML_NAMED_ELEMENT(USDef)
        QML_FOREIGN_NAMESPACE(UIShell::USDef)
        QML_SINGLETON
    };

}

#endif //UISHELL_USDEF_P_H
