// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERUTILS_H
#define DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERUTILS_H

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace PackageManager {

    struct CommandResult {
        bool ok{};
        QByteArray stdOut;
        QByteArray stdErr;
        int exitCode{};
        QString errorMessage;
    };

    CommandResult runCommandLineTool(const QString &executablePath, const QStringList &arguments, int timeoutSeconds);

}

#endif // DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERUTILS_H
