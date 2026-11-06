// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ScriptExecutionContext.h"
#include "ScriptExecutionContext_p.h"

#include <coreplugin/ActionWindowInterfaceBase.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace BatchProcess {

    ScriptExecutionContextPrivate::ScriptExecutionContextPrivate(ScriptExecutionContext *q) : q_ptr(q) {
    }

    ScriptExecutionContext::ScriptExecutionContext(QObject *parent)
        : QObject(parent), d_ptr(new ScriptExecutionContextPrivate(this)) {
    }

    ScriptExecutionContext::~ScriptExecutionContext() = default;

    QJSEngine *ScriptExecutionContext::engine() const {
        Q_D(const ScriptExecutionContext);
        return d->engine;
    }

    Script *ScriptExecutionContext::script() const {
        Q_D(const ScriptExecutionContext);
        return d->script;
    }

    ScriptAction *ScriptExecutionContext::action() const {
        Q_D(const ScriptExecutionContext);
        return d->action;
    }

    ScriptMethod *ScriptExecutionContext::method() const {
        Q_D(const ScriptExecutionContext);
        return d->method;
    }

    bool ScriptExecutionContext::isExecuting() const {
        Q_D(const ScriptExecutionContext);
        return d->action || d->method;
    }

    Core::ActionWindowInterfaceBase *ScriptExecutionContext::windowInterface() const {
        Q_D(const ScriptExecutionContext);
        return d->windowInterface;
    }

    Core::ProjectWindowInterface *ScriptExecutionContext::projectWindowInterface() const {
        Q_D(const ScriptExecutionContext);
        return d->projectWindowInterface;
    }

    DspxProject::ProjectDocumentContext *ScriptExecutionContext::projectDocumentContext() const {
        Q_D(const ScriptExecutionContext);
        return d->projectDocumentContext;
    }

}

#include "moc_ScriptExecutionContext.cpp"
