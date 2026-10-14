// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef UISHELL_WINDOWAGENT_P_H
#define UISHELL_WINDOWAGENT_P_H

#include <qqmlintegration.h>

#include <QWKQuick/quickwindowagent.h>

namespace UIShell {

    class WindowAgent : public QWK::QuickWindowAgent {
        Q_OBJECT
        QML_ELEMENT

    public:
        explicit WindowAgent(QObject *parent = nullptr);
        ~WindowAgent() override;

        Q_INVOKABLE bool setup(QQuickWindow *window);
    };

}

#endif // UISHELL_WINDOWAGENT_P_H
