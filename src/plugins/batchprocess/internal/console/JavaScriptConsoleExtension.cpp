// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JavaScriptConsoleExtension.h"

#include <QElapsedTimer>
#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QLoggingCategory>
#include <QStringList>
#include <QUrl>

#include <batchprocess/JavaScriptConsoleInterface.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/internal/JavaScriptEngineHelper.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcJavaScriptConsoleExtension, "diffscope.batchprocess.javascriptconsole.extension")

    namespace {

        constexpr auto ConsoleResourcePath = ":/diffscope/batchprocess/internalscripts/runtime/console.js";

        QString javaScriptErrorText(const QJSValue &error) {
            auto text = error.toString();
            const auto stack = error.property(QStringLiteral("stack")).toString();
            if (!stack.isEmpty() && !text.contains(stack)) {
                text.append(QLatin1Char('\n'));
                text.append(stack);
            }
            return text;
        }

    }

    class JavaScriptConsoleBridge : public QObject {
        Q_OBJECT
    public:
        JavaScriptConsoleBridge(QJSEngine *engine, JavaScriptConsoleInterface *consoleInterface, QObject *parent)
            : QObject(parent), m_engine(engine), m_consoleInterface(consoleInterface) {
            m_clock.start();
        }

        Q_INVOKABLE void appendMessage(int level, const QString &text, const QString &fileUrl, int line, int column) {
            if (level < JavaScriptConsoleInterface::Debug || level > JavaScriptConsoleInterface::Error) {
                return;
            }
            m_consoleInterface->appendMessage(static_cast<JavaScriptConsoleInterface::Level>(level), text, QUrl(fileUrl), line, column);
        }

        Q_INVOKABLE QJSValue stackTrace() const {
            const auto stackTrace = JavaScriptEngineHelper::stackTrace(m_engine);
            auto result = m_engine->newArray(static_cast<quint32>(stackTrace.size()));
            for (qsizetype index = 0; index < stackTrace.size(); ++index) {
                const auto &frame = stackTrace.at(index);
                auto item = m_engine->newObject();
                item.setProperty(QStringLiteral("functionName"), frame.functionName);
                item.setProperty(QStringLiteral("fileUrl"), frame.fileUrl.toString(QUrl::FullyEncoded));
                item.setProperty(QStringLiteral("displaySource"), frame.fileUrl.toDisplayString(QUrl::PreferLocalFile));
                item.setProperty(QStringLiteral("line"), frame.line);
                item.setProperty(QStringLiteral("column"), frame.column);
                result.setProperty(static_cast<quint32>(index), item);
            }
            return result;
        }

        Q_INVOKABLE double monotonicMilliseconds() const {
            return static_cast<double>(m_clock.nsecsElapsed()) / 1000000.0;
        }

    private:
        QJSEngine *m_engine;
        JavaScriptConsoleInterface *m_consoleInterface;
        QElapsedTimer m_clock;
    };

    JavaScriptConsoleExtension::JavaScriptConsoleExtension(BatchProcessInterface *batchProcessInterface, JavaScriptConsoleInterface *consoleInterface, QObject *parent)
        : QObject(parent), m_consoleInterface(consoleInterface) {
        if (!batchProcessInterface->registerGlobalObject(QStringLiteral("console"), this, [this](ScriptExecutionContext *context) {
                return createConsole(context);
            })) {
            qFatal("Failed to register the Batch Process JavaScript console global object");
        }
        qCDebug(lcJavaScriptConsoleExtension) << "Registered JavaScript console global object";
    }

    JavaScriptConsoleExtension::~JavaScriptConsoleExtension() = default;

    QJSValue JavaScriptConsoleExtension::createConsole(ScriptExecutionContext *context) {
        if (!context || !context->engine()) {
            qFatal("The Batch Process JavaScript console factory received an invalid execution context");
        }

        auto engine = context->engine();
        QFile consoleFile(QString::fromLatin1(ConsoleResourcePath));
        if (!consoleFile.open(QIODevice::ReadOnly)) {
            qFatal() << "Failed to open the embedded Batch Process JavaScript console:" << consoleFile.errorString();
        }

        const auto factory = engine->evaluate(QString::fromUtf8(consoleFile.readAll()), QString::fromLatin1(ConsoleResourcePath));
        if (factory.isError()) {
            qFatal() << "Failed to evaluate the embedded Batch Process JavaScript console:" << javaScriptErrorText(factory);
        }
        if (!factory.isCallable()) {
            qFatal("The embedded Batch Process JavaScript console did not return a factory function");
        }

        auto bridge = new JavaScriptConsoleBridge(engine, m_consoleInterface, engine);
        const auto console = factory.call({engine->newQObject(bridge)});
        if (engine->hasError()) {
            qFatal() << "Failed to create the Batch Process JavaScript console:" << javaScriptErrorText(engine->catchError());
        }
        if (console.isError()) {
            qFatal() << "Failed to create the Batch Process JavaScript console:" << javaScriptErrorText(console);
        }
        if (!console.isObject()) {
            qFatal("The embedded Batch Process JavaScript console factory did not return an object");
        }
        qCDebug(lcJavaScriptConsoleExtension) << "Created JavaScript console for script engine" << engine;
        return console;
    }

}

#include "JavaScriptConsoleExtension.moc"
#include "moc_JavaScriptConsoleExtension.cpp"
