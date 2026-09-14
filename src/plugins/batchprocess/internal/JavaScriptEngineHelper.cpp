// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JavaScriptEngineHelper.h"

#include <QDir>
#include <QFileInfo>
#include <QJSEngine>
#include <QtGlobal>

#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qv4engine_p.h>
#include <QtQml/private/qv4errorobject_p.h>

namespace BatchProcess::Internal {

    namespace {

        QUrl sourceUrl(const QString &source) {
            if (source.isEmpty()) {
                return {};
            }
            if (QDir::isAbsolutePath(source)) {
                return QUrl::fromLocalFile(QFileInfo(source).absoluteFilePath());
            }
            return QUrl(source);
        }

        QList<JavaScriptStackFrame> convertStackTrace(const QV4::StackTrace &stackTrace) {
            QList<JavaScriptStackFrame> result;
            result.reserve(stackTrace.size());
            for (const auto &frame : stackTrace) {
                const auto line = frame.line == -1 ? -1 : qAbs(frame.line);
                result.append({frame.function, sourceUrl(frame.source), line, frame.column});
            }
            return result;
        }

        QString stackFrameSource(const QUrl &fileUrl) {
            return fileUrl.isLocalFile() ? QFileInfo(fileUrl.toLocalFile()).absoluteFilePath() : fileUrl.toString(QUrl::FullyEncoded);
        }

        QV4::StackTrace convertStackTrace(const QList<JavaScriptStackFrame> &stackTrace) {
            QV4::StackTrace result;
            result.reserve(stackTrace.size());
            for (const auto &frame : stackTrace) {
                result.append({stackFrameSource(frame.fileUrl), frame.functionName, frame.line, frame.column});
            }
            return result;
        }

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
