// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTMETHOD_P_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTMETHOD_P_H

#include <batchprocess/ScriptMethod.h>

#include <QJSValue>

namespace BatchProcess {

    class ScriptMethodPrivate {
        Q_DECLARE_PUBLIC(ScriptMethod)
    public:
        explicit ScriptMethodPrivate(ScriptMethod *q);

        ScriptMethod *q_ptr;
        Script *script{};
        QJsonObject metadata;
        QJSValue executeFunction;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTMETHOD_P_H
