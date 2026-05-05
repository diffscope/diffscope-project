#include "PackageManagerUtils.h"

#include <algorithm>

#include <QCoreApplication>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QProcess>
#include <QTimer>
#include <QtGlobal>

namespace PackageManager {

    Q_STATIC_LOGGING_CATEGORY(lcPackageManagerUtils, "diffscope.packagemanager.utils")

    namespace {

        void logRawOutput(const CommandResult &result) {
            qCDebug(lcPackageManagerUtils).noquote() << "Command stdout:" << QString::fromUtf8(result.stdOut);
            qCDebug(lcPackageManagerUtils).noquote() << "Command stderr:" << QString::fromUtf8(result.stdErr);
        }

    }

    CommandResult runCommandLineTool(const QString &executablePath, const QStringList &arguments, int timeoutSeconds) {
        CommandResult result;
        QProcess process;
        process.setProgram(executablePath);
        process.setArguments(arguments);

        qCInfo(lcPackageManagerUtils) << "Running command line tool" << executablePath << arguments;

        const int timeoutMs = std::max(1, timeoutSeconds) * 1000;
        QEventLoop eventLoop;
        QTimer timer;
        timer.setSingleShot(true);
        bool timedOut = false;
        bool processError = false;
        bool processFinished = false;

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

        process.start();
        timer.start(timeoutMs);
        eventLoop.exec();
        timer.stop();

        if (timedOut && process.state() != QProcess::NotRunning) {
            QEventLoop killLoop;
            QTimer killTimer;
            killTimer.setSingleShot(true);
            QObject::connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), &killLoop, SLOT(quit()));
            QObject::connect(&killTimer, &QTimer::timeout, &killLoop, &QEventLoop::quit);
            killTimer.start(1000);
            killLoop.exec();
        }

        result.stdOut = process.readAllStandardOutput();
        result.stdErr = process.readAllStandardError();
        result.exitCode = process.exitCode();
        logRawOutput(result);

        if (timedOut) {
            result.errorMessage = QCoreApplication::translate("PackageManager::CommandLineTool", "The command timed out.");
            return result;
        }
        if (processError && !processFinished) {
            result.errorMessage = process.errorString();
            return result;
        }
        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            result.errorMessage = QString::fromLocal8Bit(result.stdErr).trimmed();
            if (result.errorMessage.isEmpty()) {
                result.errorMessage = QString::fromLocal8Bit(result.stdOut).trimmed();
            }
            if (result.errorMessage.isEmpty()) {
                result.errorMessage = QCoreApplication::translate("PackageManager::CommandLineTool", "The command exited with code %1.").arg(process.exitCode());
            }
            return result;
        }

        result.ok = true;
        return result;
    }

}
