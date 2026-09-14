// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ScriptAction.h"
#include "ScriptAction_p.h"

namespace BatchProcess {

    ScriptActionPrivate::ScriptActionPrivate(ScriptAction *q) : q_ptr(q) {
    }

    ScriptAction::ScriptAction(Script *script, QObject *parent)
        : QObject(parent), d_ptr(new ScriptActionPrivate(this)) {
        Q_D(ScriptAction);
        d->script = script;
    }

    ScriptAction::~ScriptAction() = default;

    Script *ScriptAction::script() const {
        Q_D(const ScriptAction);
        return d->script;
    }

    QString ScriptAction::name() const {
        Q_D(const ScriptAction);
        return d->name;
    }

    QString ScriptAction::description() const {
        Q_D(const ScriptAction);
        return d->description;
    }

    bool ScriptAction::requiresProject() const {
        Q_D(const ScriptAction);
        return d->requiresProject;
    }

    QJSValue ScriptAction::executeFunction() const {
        Q_D(const ScriptAction);
        return d->executeFunction;
    }

}

#include "moc_ScriptAction.cpp"
