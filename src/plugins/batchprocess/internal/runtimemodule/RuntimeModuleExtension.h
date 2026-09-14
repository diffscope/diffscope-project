// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_RUNTIMEMODULEEXTENSION_H
#define DIFFSCOPE_BATCHPROCESS_RUNTIMEMODULEEXTENSION_H

#include <QObject>

class QJSEngine;
class QJSValue;

namespace BatchProcess {
    class BatchProcessInterface;
    class ScriptExecutionContext;
}

namespace BatchProcess::Internal {

    class RuntimeModuleBridge;

    class RuntimeModuleExtension : public QObject {
        Q_OBJECT
    public:
        explicit RuntimeModuleExtension(BatchProcessInterface *batchProcessInterface, QObject *parent = nullptr);
        ~RuntimeModuleExtension() override;

    private:
        friend class RuntimeModuleBridge;

        QJSValue createModule(ScriptExecutionContext *context);
        QJSValue createScriptRuntime(ScriptExecutionContext *context, QJSEngine *engine) const;
        void abort(ScriptExecutionContext *context, QJSEngine *engine) const;

        BatchProcessInterface *m_batchProcessInterface;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_RUNTIMEMODULEEXTENSION_H
