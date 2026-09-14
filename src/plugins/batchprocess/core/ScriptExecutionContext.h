// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTEXECUTIONCONTEXT_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTEXECUTIONCONTEXT_H

#include <QObject>

#include <batchprocess/batchprocessglobal.h>

class QJSEngine;

namespace Core {
    class ActionWindowInterfaceBase;
    class ProjectDocumentContext;
    class ProjectWindowInterface;
}

namespace BatchProcess {

    namespace Internal {
        class BatchProcessRuntime;
    }

    class Script;
    class ScriptAction;
    class ScriptExecutionContextPrivate;

    class BATCH_PROCESS_EXPORT ScriptExecutionContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ScriptExecutionContext)
    public:
        ~ScriptExecutionContext() override;

        QJSEngine *engine() const;
        Script *script() const;
        ScriptAction *action() const;
        Core::ActionWindowInterfaceBase *windowInterface() const;
        Core::ProjectWindowInterface *projectWindowInterface() const;
        Core::ProjectDocumentContext *projectDocumentContext() const;

    private:
        friend class Internal::BatchProcessRuntime;
        explicit ScriptExecutionContext(QObject *parent = nullptr);

        QScopedPointer<ScriptExecutionContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTEXECUTIONCONTEXT_H
