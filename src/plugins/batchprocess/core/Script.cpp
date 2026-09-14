// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Script.h"
#include "Script_p.h"

namespace BatchProcess {

    ScriptPrivate::ScriptPrivate(Script *q) : q_ptr(q) {
    }

    Script::Script(const QString &filePath, const QString &rootPath, QObject *parent)
        : QObject(parent), d_ptr(new ScriptPrivate(this)) {
        Q_D(Script);
        d->filePath = filePath;
        d->rootPath = rootPath;
    }

    Script::~Script() = default;

    QString Script::filePath() const {
        Q_D(const Script);
        return d->filePath;
    }

    ScriptMetadata Script::metadata() const {
        Q_D(const Script);
        return d->metadata;
    }

    QList<ScriptAction *> Script::actions() const {
        Q_D(const Script);
        return d->actions;
    }

}

#include "moc_Script.cpp"
