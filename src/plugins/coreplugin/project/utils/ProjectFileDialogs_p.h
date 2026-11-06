// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTFILEDIALOGS_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTFILEDIALOGS_P_H

#include <coreplugin/ProjectFileDialogs.h>

#include <QPointer>

namespace Core {

    class ProjectFileDialogsPrivate {
    public:
        void reportParseError(const QString &path);

        QPointer<QWindow> window;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROJECTFILEDIALOGS_P_H
