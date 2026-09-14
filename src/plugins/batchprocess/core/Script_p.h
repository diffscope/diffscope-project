// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPT_P_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPT_P_H

#include <batchprocess/Script.h>

namespace BatchProcess {

    class ScriptPrivate {
        Q_DECLARE_PUBLIC(Script)
    public:
        explicit ScriptPrivate(Script *q);

        Script *q_ptr;
        QString filePath;
        QString rootPath;
        ScriptMetadata metadata;
        QList<ScriptAction *> actions;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPT_P_H
