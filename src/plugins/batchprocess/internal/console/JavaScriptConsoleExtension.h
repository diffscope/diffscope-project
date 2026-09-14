// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEEXTENSION_H
#define DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEEXTENSION_H

#include <QObject>

#include <batchprocess/BatchProcessInterface.h>

namespace BatchProcess {
    class JavaScriptConsoleInterface;
    class ScriptExecutionContext;
}

namespace BatchProcess::Internal {

    class JavaScriptConsoleExtension : public QObject {
        Q_OBJECT
    public:
        JavaScriptConsoleExtension(BatchProcessInterface *batchProcessInterface, JavaScriptConsoleInterface *consoleInterface, QObject *parent = nullptr);
        ~JavaScriptConsoleExtension() override;

    private:
        QJSValue createConsole(ScriptExecutionContext *context);

        JavaScriptConsoleInterface *m_consoleInterface;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEEXTENSION_H
