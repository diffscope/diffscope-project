// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ScriptMethod.h"
#include "ScriptMethod_p.h"

namespace BatchProcess {

    ScriptMethodPrivate::ScriptMethodPrivate(ScriptMethod *q) : q_ptr(q) {
    }

    ScriptMethod::ScriptMethod(Script *script, QObject *parent)
        : QObject(parent), d_ptr(new ScriptMethodPrivate(this)) {
        Q_D(ScriptMethod);
        d->script = script;
    }

    ScriptMethod::~ScriptMethod() = default;

    Script *ScriptMethod::script() const {
        Q_D(const ScriptMethod);
        return d->script;
    }

    QString ScriptMethod::name() const {
        Q_D(const ScriptMethod);
        return d->metadata.value(QStringLiteral("name")).toString();
    }

    QString ScriptMethod::summary() const {
        Q_D(const ScriptMethod);
        return d->metadata.value(QStringLiteral("summary")).toString();
    }

    QString ScriptMethod::description() const {
        Q_D(const ScriptMethod);
        return d->metadata.value(QStringLiteral("description")).toString();
    }

    QJsonArray ScriptMethod::params() const {
        Q_D(const ScriptMethod);
        return d->metadata.value(QStringLiteral("params")).toArray();
    }

    QJsonValue ScriptMethod::result() const {
        Q_D(const ScriptMethod);
        return d->metadata.value(QStringLiteral("result"));
    }

    QJsonObject ScriptMethod::annotations() const {
        Q_D(const ScriptMethod);
        return d->metadata.value(QStringLiteral("annotations")).toObject();
    }

    QJsonObject ScriptMethod::metadata() const {
        Q_D(const ScriptMethod);
        return d->metadata;
    }

    QJSValue ScriptMethod::executeFunction() const {
        Q_D(const ScriptMethod);
        return d->executeFunction;
    }

}

#include "moc_ScriptMethod.cpp"
