// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTEXECUTIONCONTEXT_P_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTEXECUTIONCONTEXT_P_H

#include <batchprocess/ScriptExecutionContext.h>

#include <QPointer>

namespace BatchProcess {

    class ScriptExecutionContextPrivate {
        Q_DECLARE_PUBLIC(ScriptExecutionContext)
    public:
        explicit ScriptExecutionContextPrivate(ScriptExecutionContext *q);

        ScriptExecutionContext *q_ptr;
        QJSEngine *engine{};
        Script *script{};
        ScriptAction *action{};
        QPointer<Core::ActionWindowInterfaceBase> windowInterface;
        QPointer<Core::ProjectWindowInterface> projectWindowInterface;
        QPointer<Core::ProjectDocumentContext> projectDocumentContext;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTEXECUTIONCONTEXT_P_H
