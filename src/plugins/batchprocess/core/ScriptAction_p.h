// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTACTION_P_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTACTION_P_H

#include <batchprocess/ScriptAction.h>

#include <QJSValue>

namespace BatchProcess {

    class ScriptActionPrivate {
        Q_DECLARE_PUBLIC(ScriptAction)
    public:
        explicit ScriptActionPrivate(ScriptAction *q);

        ScriptAction *q_ptr;
        Script *script{};
        QString name;
        QString description;
        bool requiresProject{};
        QJSValue executeFunction;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTACTION_P_H
