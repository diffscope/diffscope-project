// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JavaScriptEngineHelper.h"

#include <QDir>
#include <QJSEngine>
#include <QStringList>
#include <QtGlobal>

#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qv4engine_p.h>
#include <QtQml/private/qv4errorobject_p.h>

namespace BatchProcess::Internal {

    namespace {

        QList<JavaScriptStackFrame> convertStackTrace(const QV4::StackTrace &stackTrace) {
            QList<JavaScriptStackFrame> result;
            result.reserve(stackTrace.size());
            for (const auto &frame : stackTrace) {
                const auto line = frame.line == -1 ? -1 : qAbs(frame.line);
                result.append({frame.function, QUrl(frame.source), line, frame.column});
            }
            return result;
        }

        QV4::StackTrace convertStackTrace(const QList<JavaScriptStackFrame> &stackTrace) {
            QV4::StackTrace result;
            result.reserve(stackTrace.size());
            for (const auto &frame : stackTrace) {
                result.append({frame.fileUrl.toString(), frame.functionName, frame.line, frame.column});
            }
            return result;
        }

    }

    QUrl JavaScriptEngineHelper::filePathToSourceUrl(const QString &filePath) {
        if (!filePath.startsWith(QLatin1String(":/"))) {
            return QUrl::fromLocalFile(filePath);
        }
        QUrl fileUrl;
        fileUrl.setScheme(QStringLiteral("qrc"));
        fileUrl.setPath(filePath.mid(1), QUrl::DecodedMode);
        return fileUrl;
    }

    QString JavaScriptEngineHelper::sourceDisplayPath(const QUrl &fileUrl) {
        return fileUrl.isLocalFile() ? QDir::toNativeSeparators(fileUrl.toLocalFile()) : fileUrl.toDisplayString();
    }

    QString JavaScriptEngineHelper::formatStackTrace(const QList<JavaScriptStackFrame> &stackTrace) {
        QStringList lines;
        lines.reserve(stackTrace.size());
        for (const auto &frame : stackTrace) {
            auto text = frame.functionName + QLatin1Char('@') + sourceDisplayPath(frame.fileUrl);
            if (frame.line > 0) {
                text += QLatin1Char(':') + QString::number(frame.line);
            }
            lines.append(text);
        }
        return lines.join(QLatin1Char('\n'));
    }

    QList<JavaScriptStackFrame> JavaScriptEngineHelper::stackTrace(QJSEngine *engine, int frameLimit) {
        if (!engine || !engine->handle()) {
            return {};
        }

        return convertStackTrace(engine->handle()->stackTrace(frameLimit));
    }

    QList<JavaScriptStackFrame> JavaScriptEngineHelper::stackTrace(const QJSValue &error) {
        const auto errorObject = QJSValuePrivate::asManagedType<QV4::ErrorObject>(&error);
        if (!errorObject || !errorObject->d()->stackTrace) {
            return {};
        }
        return convertStackTrace(*errorObject->d()->stackTrace);
    }

    QList<JavaScriptStackFrame> JavaScriptEngineHelper::takeExceptionStackTrace(QJSEngine *engine) {
        if (!engine || !engine->handle()) {
            return {};
        }
        auto result = convertStackTrace(engine->handle()->exceptionStackTrace);
        engine->handle()->exceptionStackTrace.clear();
        return result;
    }

    void JavaScriptEngineHelper::throwError(QJSEngine *engine, const QJSValue &error, const QList<JavaScriptStackFrame> &stackTrace) {
        if (!engine || !engine->handle()) {
            return;
        }
        engine->throwError(error);
        if (!stackTrace.isEmpty()) {
            engine->handle()->exceptionStackTrace = convertStackTrace(stackTrace);
        }
    }

}
