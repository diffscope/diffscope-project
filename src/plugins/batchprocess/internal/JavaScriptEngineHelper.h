// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_JAVASCRIPTENGINEHELPER_H
#define DIFFSCOPE_BATCHPROCESS_JAVASCRIPTENGINEHELPER_H

#include <QList>
#include <QString>
#include <QUrl>

class QJSEngine;
class QJSValue;

namespace BatchProcess::Internal {

    struct JavaScriptStackFrame {
        QString functionName;
        QUrl fileUrl;
        int line{-1};
        int column{-1};
    };

    class JavaScriptEngineHelper {
    public:
        static QList<JavaScriptStackFrame> stackTrace(QJSEngine *engine, int frameLimit = -1);
        static QList<JavaScriptStackFrame> stackTrace(const QJSValue &error);
        static QList<JavaScriptStackFrame> takeExceptionStackTrace(QJSEngine *engine);
        static void throwError(QJSEngine *engine, const QJSValue &error, const QList<JavaScriptStackFrame> &stackTrace);
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_JAVASCRIPTENGINEHELPER_H
