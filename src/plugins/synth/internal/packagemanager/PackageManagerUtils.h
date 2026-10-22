// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_PACKAGEMANAGERUTILS_H
#define DIFFSCOPE_SYNTH_INTERNAL_PACKAGEMANAGERUTILS_H

#include <functional>

#include <QByteArray>
#include <QJsonValue>
#include <QString>
#include <QStringList>

namespace Synth::Internal {

    struct CommandResult {
        bool ok{};
        bool cancelled{};
        QByteArray stdOut;
        QByteArray stdErr;
        int exitCode{};
        QString errorMessage;
    };

    struct DspmResult {
        bool ok{};
        QString command;
        QJsonValue data;
        QString errorCode;
        QString errorMessage;
    };

    // Runs a command with a timeout measured from its most recent standard output or standard error.
    CommandResult runCommandLineTool(const QString &executablePath, const QStringList &arguments, int timeoutSeconds, const std::function<bool()> &isCancelled = {}, const std::function<void(const QByteArray &)> &outputReceived = {});
    CommandResult validateDspmExecutable(const QString &path, int timeoutSeconds, const std::function<bool()> &isCancelled = {});
    DspmResult parseDspmQueryResult(const CommandResult &command);
    DspmResult parseDspmMutationResult(const CommandResult &command, const QString &operation, bool dryRun);
    QString localizedPackageText(const QJsonValue &value);

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_PACKAGEMANAGERUTILS_H
