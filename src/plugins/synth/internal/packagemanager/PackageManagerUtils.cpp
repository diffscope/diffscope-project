// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageManagerUtils.h"

#include <algorithm>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QLoggingCategory>
#include <QProcess>
#include <QTimer>
#include <QVariant>
#include <QtGlobal>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcPackageManagerUtils, "diffscope.synth.utils")

    namespace {

        void logRawOutput(const CommandResult &result) {
            qCDebug(lcPackageManagerUtils).noquote() << "Command stdout:" << QString::fromUtf8(result.stdOut);
            qCDebug(lcPackageManagerUtils).noquote() << "Command stderr:" << QString::fromUtf8(result.stdErr);
        }

        void readDspmError(const QJsonObject &record, DspmResult *result) {
            const auto error = record.value(QStringLiteral("error")).toObject();
            result->errorCode = error.value(QStringLiteral("code")).toString();
            result->errorMessage = error.value(QStringLiteral("message")).toString();
            if (!result->errorCode.isEmpty() && !result->errorMessage.isEmpty())
                result->errorMessage = QStringLiteral("%1: %2").arg(result->errorCode, result->errorMessage);
            const auto details = error.value(QStringLiteral("details"));
            if (details.isObject())
                result->errorMessage.append(u'\n' + QString::fromUtf8(QJsonDocument(details.toObject()).toJson(QJsonDocument::Indented)));
            else if (details.isArray())
                result->errorMessage.append(u'\n' + QString::fromUtf8(QJsonDocument(details.toArray()).toJson(QJsonDocument::Indented)));
            else if (details.isString())
                result->errorMessage.append(u'\n' + details.toString());
            else if (!details.isUndefined() && !details.isNull())
                result->errorMessage.append(u'\n' + details.toVariant().toString());
            for (const auto &diagnostic : record.value(QStringLiteral("diagnostics")).toArray())
                if (diagnostic.isString() && !diagnostic.toString().isEmpty())
                    result->errorMessage.append(u'\n' + diagnostic.toString());
            const auto diagnosticDetails = record.value(QStringLiteral("diagnosticDetails")).toArray();
            if (!diagnosticDetails.isEmpty())
                result->errorMessage.append(u'\n' + QString::fromUtf8(QJsonDocument(diagnosticDetails).toJson(QJsonDocument::Indented)));
        }

        void appendCommandError(const CommandResult &command, DspmResult *result) {
            if (result->errorMessage.isEmpty()) {
                result->errorMessage = command.errorMessage;
                return;
            }
            const auto standardError = QString::fromUtf8(command.stdErr).trimmed();
            if (!standardError.isEmpty() && !result->errorMessage.contains(standardError))
                result->errorMessage.append(QStringLiteral("\n\n") + standardError);
            if (!command.ok && !command.errorMessage.isEmpty() && command.errorMessage != QString::fromUtf8(command.stdOut).trimmed() && !result->errorMessage.contains(command.errorMessage))
                result->errorMessage.append(QStringLiteral("\n\n") + command.errorMessage);
        }

    }

    CommandResult runCommandLineTool(const QString &executablePath, const QStringList &arguments, int timeoutSeconds, const std::function<bool()> &isCancelled, const std::function<void(const QByteArray &)> &outputReceived) {
        CommandResult result;
        QProcess process;

        if (isCancelled && isCancelled()) {
            result.cancelled = true;
            return result;
        }
        if (executablePath.isEmpty()) {
            result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "No package manager installation is selected. Configure it in Settings > Synthesis > Package Manager.");
            return result;
        }

        if (!QFileInfo(executablePath).isFile()) {
            result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "The command line tool does not exist:\n\n%1\n\nPlease install dspm and configure it in Settings > Synthesis > Package Manager.").arg(executablePath);
            return result;
        }

        process.setProgram(executablePath);
        process.setArguments(arguments);

        qCInfo(lcPackageManagerUtils) << "Running command line tool" << executablePath << arguments;

        const int timeoutMs = std::max(1, timeoutSeconds) * 1000;
        QEventLoop eventLoop;
        QTimer timer;
        QTimer cancellationTimer;
        timer.setSingleShot(true);
        timer.setTimerType(Qt::PreciseTimer);
        bool timedOut = false;
        bool processError = false;
        bool processFinished = false;
        const auto readOutput = [&] {
            const auto output = process.readAllStandardOutput();
            if (output.isEmpty())
                return;
            result.stdOut.append(output);
            timer.start(timeoutMs);
            if (outputReceived)
                outputReceived(output);
        };
        const auto readError = [&] {
            const auto output = process.readAllStandardError();
            if (output.isEmpty())
                return;
            result.stdErr.append(output);
            timer.start(timeoutMs);
        };

        QObject::connect(&process, &QProcess::readyReadStandardOutput, &eventLoop, readOutput);
        QObject::connect(&process, &QProcess::readyReadStandardError, &eventLoop, readError);
        QObject::connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), &eventLoop, SLOT(quit()));
        QObject::connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &process, [&processFinished](int, QProcess::ExitStatus) {
            processFinished = true;
        });
        QObject::connect(&process, &QProcess::errorOccurred, &eventLoop, &QEventLoop::quit);
        QObject::connect(&process, &QProcess::errorOccurred, &process, [&processError]() {
            processError = true;
        });
        QObject::connect(&timer, &QTimer::timeout, &eventLoop, [&]() {
            timedOut = true;
            if (process.state() != QProcess::NotRunning) {
                process.kill();
            }
            eventLoop.quit();
        });
        if (isCancelled) {
            QObject::connect(&cancellationTimer, &QTimer::timeout, &eventLoop, [&] {
                if (!isCancelled())
                    return;
                result.cancelled = true;
                if (process.state() != QProcess::NotRunning)
                    process.kill();
                eventLoop.quit();
            });
            cancellationTimer.start(50);
        }

        process.start();
        timer.start(timeoutMs);
        eventLoop.exec();
        cancellationTimer.stop();

        if ((timedOut || result.cancelled) && process.state() != QProcess::NotRunning) {
            QEventLoop killLoop;
            QTimer killTimer;
            killTimer.setSingleShot(true);
            QObject::connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), &killLoop, SLOT(quit()));
            QObject::connect(&killTimer, &QTimer::timeout, &killLoop, &QEventLoop::quit);
            killTimer.start(1000);
            killLoop.exec();
        }

        readOutput();
        readError();
        timer.stop();
        result.exitCode = process.exitCode();
        logRawOutput(result);

        if (result.cancelled)
            return result;
        if (timedOut) {
            result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "The command produced no output for %L1 seconds.").arg(std::max(1, timeoutSeconds));
            return result;
        }
        if (processError && !processFinished) {
            result.errorMessage = process.errorString();
            return result;
        }
        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            result.errorMessage = QString::fromUtf8(result.stdErr).trimmed();
            if (result.errorMessage.isEmpty()) {
                result.errorMessage = QString::fromUtf8(result.stdOut).trimmed();
            }
            if (result.errorMessage.isEmpty()) {
                result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "The command exited with code %1.").arg(process.exitCode());
            }
            return result;
        }

        result.ok = true;
        return result;
    }

    CommandResult validateDspmExecutable(const QString &path, int timeoutSeconds, const std::function<bool()> &isCancelled) {
        auto result = runCommandLineTool(path, {QStringLiteral("--json"), QStringLiteral("--version")}, timeoutSeconds, isCancelled);
        if (!result.ok)
            return result;
        const auto document = QJsonDocument::fromJson(result.stdOut);
        result.ok = false;
        if (!document.isObject()) {
            result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "The command did not return valid JSON.");
        } else if (document.object().value(QStringLiteral("name")).toString() != QStringLiteral("dspm") || !document.object().value(QStringLiteral("version")).isString()) {
            result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "The selected command is not dspm.");
        } else {
            result.ok = true;
        }
        return result;
    }

    DspmResult parseDspmQueryResult(const CommandResult &command) {
        DspmResult result;
        const auto document = QJsonDocument::fromJson(command.stdOut);
        const auto root = document.object();
        if (!command.ok || !root.value(QStringLiteral("ok")).toBool()) {
            readDspmError(root, &result);
            appendCommandError(command, &result);
            if (result.errorMessage.isEmpty())
                result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "The command did not return valid JSON.");
            return result;
        }
        result.data = root.value(QStringLiteral("data"));
        result.command = root.value(QStringLiteral("command")).toString();
        result.ok = !result.data.isUndefined();
        if (!result.ok)
            result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "The command did not return valid JSON.");
        return result;
    }

    DspmResult parseDspmMutationResult(const CommandResult &command, const QString &operation, bool dryRun) {
        DspmResult result;
        bool summaryReceived = false;
        bool errorReceived = false;
        QString invalidOutput;
        int lineNumber = 0;
        for (const auto &line : command.stdOut.split('\n')) {
            ++lineNumber;
            if (line.trimmed().isEmpty())
                continue;
            QJsonParseError parseError;
            const auto document = QJsonDocument::fromJson(line, &parseError);
            if (!document.isObject()) {
                invalidOutput = QCoreApplication::translate("Synth::Internal::CommandLineTool", "Invalid JSON at output line %L1: %2\n%3").arg(lineNumber).arg(parseError.errorString(), QString::fromUtf8(line));
                break;
            }
            if (summaryReceived || errorReceived) {
                invalidOutput = QCoreApplication::translate("Synth::Internal::CommandLineTool", "Output followed the final operation record:\n%1").arg(QString::fromUtf8(line));
                break;
            }
            const auto record = document.object();
            const auto event = record.value(QStringLiteral("event")).toString();
            if (event == QStringLiteral("error")) {
                readDspmError(record, &result);
                errorReceived = true;
                continue;
            }
            if (record.value(QStringLiteral("command")).toString() != operation || !record.contains(QStringLiteral("data")) || record.contains(QStringLiteral("ok")) || record.contains(QStringLiteral("error"))) {
                invalidOutput = QCoreApplication::translate("Synth::Internal::CommandLineTool", "Unexpected operation record:\n%1").arg(QString::fromUtf8(line));
                break;
            }
            if (event == QStringLiteral("summary")) {
                summaryReceived = true;
                result.data = record.value(QStringLiteral("data"));
            } else if (event != QStringLiteral("progress") && event != QStringLiteral("plan") && event != QStringLiteral("result") &&
                       !(operation == QStringLiteral("install") && (event == QStringLiteral("inspect_start") || event == QStringLiteral("inspect_ok") || event == QStringLiteral("staged")))) {
                invalidOutput = QCoreApplication::translate("Synth::Internal::CommandLineTool", "Unexpected operation record:\n%1").arg(QString::fromUtf8(line));
                break;
            }
        }
        if (!command.ok || errorReceived) {
            appendCommandError(command, &result);
            if (result.errorMessage.isEmpty())
                result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "The command reported an error.");
            return result;
        }
        const auto summary = result.data.toObject();
        const auto dryRunValue = summary.value(QStringLiteral("dryRun"));
        result.ok = invalidOutput.isEmpty() && summaryReceived && dryRunValue.isBool() && dryRunValue.toBool() == dryRun && summary.value(QStringLiteral("actions")).isArray() && !summary.contains(QStringLiteral("ignored"));
        if (!result.ok) {
            if (invalidOutput.isEmpty())
                invalidOutput = summaryReceived
                    ? QCoreApplication::translate("Synth::Internal::CommandLineTool", "Invalid operation summary:\n%1").arg(QString::fromUtf8(QJsonDocument(summary).toJson(QJsonDocument::Indented)))
                    : QCoreApplication::translate("Synth::Internal::CommandLineTool", "The command completed without an operation summary.");
            result.errorMessage = QCoreApplication::translate("Synth::Internal::CommandLineTool", "The package manager returned an invalid operation result:\n%1").arg(invalidOutput);
            appendCommandError(command, &result);
        }
        return result;
    }

    QString localizedPackageText(const QJsonValue &value) {
        const auto object = value.toObject();
        const auto languages = QLocale().uiLanguages();
        for (auto language : languages) {
            language.replace(u'_', u'-');
            for (auto it = object.begin(); it != object.end(); ++it) {
                auto key = it.key();
                key.replace(u'_', u'-');
                if (key.compare(language, Qt::CaseInsensitive) == 0)
                    return it.value().toString();
            }
        }
        return object.value(QStringLiteral("_")).toString();
    }

}
