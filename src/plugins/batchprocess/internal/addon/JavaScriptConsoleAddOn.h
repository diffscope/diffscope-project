// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEADDON_H
#define DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace BatchProcess::Internal {

    class JavaScriptDebugConsoleDialog;

    class JavaScriptConsoleAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit JavaScriptConsoleAddOn(QObject *parent = nullptr);
        ~JavaScriptConsoleAddOn() override;

        Q_INVOKABLE void showConsole();

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        JavaScriptDebugConsoleDialog *m_dialog{};
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEADDON_H
