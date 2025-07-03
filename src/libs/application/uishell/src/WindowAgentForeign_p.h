#ifndef UISHELL_WINDOWAGENTFOREIGN_P_H
#define UISHELL_WINDOWAGENTFOREIGN_P_H

#include <QObject>
#include <qqmlintegration.h>

#include <QWKQuick/quickwindowagent.h>

namespace UIShell {

    class WindowAgentForeign : public QObject {
        Q_OBJECT
        QML_NAMED_ELEMENT(WindowAgent)
        QML_FOREIGN(QWK::QuickWindowAgent)
    };

}

#endif //UISHELL_WINDOWAGENTFOREIGN_P_H
