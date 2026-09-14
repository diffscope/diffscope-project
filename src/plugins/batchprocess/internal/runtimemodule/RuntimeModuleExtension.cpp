// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "RuntimeModuleExtension.h"

#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QLocale>
#include <QLoggingCategory>
#include <QPointer>
#include <QtGlobal>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/Script.h>
#include <batchprocess/ScriptAction.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/private/Script_p.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcRuntimeModuleExtension, "diffscope.batchprocess.runtimemodule.extension")

    namespace {

        constexpr auto RuntimeModuleName = "diffscope:runtime";
        constexpr auto RuntimeModuleResourcePath = ":/diffscope/batchprocess/internalscripts/runtime/runtime-module.js";

        QString javaScriptErrorText(const QJSValue &error) {
            auto text = error.toString();
            const auto stack = error.property(QStringLiteral("stack")).toString();
            if (!stack.isEmpty() && !text.contains(stack)) {
                text.append(QLatin1Char('\n'));
                text.append(stack);
            }
            return text;
        }

        QString platformName() {
#if defined(Q_OS_WIN)
            return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
            return QStringLiteral("macos");
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
            return QStringLiteral("linux");
#else
#error Unsupported platform for diffscope:runtime
#endif
        }

    }

    class RuntimeModuleBridge : public QObject {
        Q_OBJECT
    public:
        RuntimeModuleBridge(RuntimeModuleExtension *extension, ScriptExecutionContext *context, QJSEngine *engine, QObject *parent)
            : QObject(parent), m_extension(extension), m_context(context), m_engine(engine) {
        }

        Q_INVOKABLE QJSValue currentScriptRuntime() const {
            if (!m_extension) {
                m_engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("The runtime module is no longer available."));
                return QJSValue(QJSValue::UndefinedValue);
            }
            return m_extension->createScriptRuntime(m_context.data(), m_engine);
        }

        Q_INVOKABLE void abort() const {
            if (!m_extension) {
                m_engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("The runtime module is no longer available."));
                return;
            }
            m_extension->abort(m_context.data(), m_engine);
        }

    private:
        QPointer<RuntimeModuleExtension> m_extension;
        QPointer<ScriptExecutionContext> m_context;
        QJSEngine *m_engine;
    };

    RuntimeModuleExtension::RuntimeModuleExtension(BatchProcessInterface *batchProcessInterface, QObject *parent)
        : QObject(parent), m_batchProcessInterface(batchProcessInterface) {
        Q_ASSERT(m_batchProcessInterface);
        if (!m_batchProcessInterface->registerModule(QString::fromLatin1(RuntimeModuleName), this, [this](ScriptExecutionContext *context) {
                return createModule(context);
            })) {
            qFatal("Failed to register the Batch Process runtime module");
        }
        qCDebug(lcRuntimeModuleExtension) << "Registered runtime module" << RuntimeModuleName;
    }

    RuntimeModuleExtension::~RuntimeModuleExtension() = default;

    QJSValue RuntimeModuleExtension::createModule(ScriptExecutionContext *context) {
        if (!context || !context->engine()) {
            qFatal("The Batch Process runtime module factory received an invalid execution context");
        }

        auto engine = context->engine();
        QFile runtimeModuleFile(QString::fromLatin1(RuntimeModuleResourcePath));
        if (!runtimeModuleFile.open(QIODevice::ReadOnly)) {
            qFatal() << "Failed to open the embedded Batch Process runtime module:" << runtimeModuleFile.errorString();
        }

        const auto factory = engine->evaluate(QString::fromUtf8(runtimeModuleFile.readAll()), QString::fromLatin1(RuntimeModuleResourcePath));
        if (factory.isError()) {
            qFatal() << "Failed to evaluate the embedded Batch Process runtime module:" << javaScriptErrorText(factory);
        }
        if (!factory.isCallable()) {
            qFatal("The embedded Batch Process runtime module did not return a factory function");
        }

        auto bridge = new RuntimeModuleBridge(this, context, engine, engine);
        const auto runtimeModule = factory.call({engine->newQObject(bridge)});
        if (engine->hasError()) {
            qFatal() << "Failed to create the Batch Process runtime module:" << javaScriptErrorText(engine->catchError());
        }
        if (runtimeModule.isError()) {
            qFatal() << "Failed to create the Batch Process runtime module:" << javaScriptErrorText(runtimeModule);
        }
        if (!runtimeModule.isObject()) {
            qFatal("The embedded Batch Process runtime module factory returned an invalid module object");
        }
        qCDebug(lcRuntimeModuleExtension) << "Created runtime module for script engine" << engine;
        return runtimeModule;
    }

    QJSValue RuntimeModuleExtension::createScriptRuntime(ScriptExecutionContext *context, QJSEngine *engine) const {
        if (!context || context->engine() != engine || !context->script() || !context->action()) {
            engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("Runtime script information is only available while a script action is executing."));
            return QJSValue(QJSValue::UndefinedValue);
        }

        const auto script = context->script();
        const auto action = context->action();
        const auto actions = script->actions();
        const auto actionIndex = actions.indexOf(action);
        if (actionIndex < 0) {
            engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("The current script action is not part of the current script."));
            return QJSValue(QJSValue::UndefinedValue);
        }

        auto metadata = engine->newObject();
        const auto scriptMetadata = script->metadata();
        metadata.setProperty(QStringLiteral("id"), scriptMetadata.id());
        metadata.setProperty(QStringLiteral("name"), scriptMetadata.name());
        metadata.setProperty(QStringLiteral("version"), scriptMetadata.version());
        metadata.setProperty(QStringLiteral("description"), scriptMetadata.description());
        if (!scriptMetadata.author().isEmpty()) {
            metadata.setProperty(QStringLiteral("author"), scriptMetadata.author());
        }

        auto actionMetadataArray = engine->newArray(static_cast<quint32>(actions.size()));
        QJSValue currentActionMetadata;
        for (qsizetype index = 0; index < actions.size(); ++index) {
            const auto scriptAction = actions.at(index);
            auto actionMetadata = engine->newObject();
            actionMetadata.setProperty(QStringLiteral("name"), scriptAction->name());
            actionMetadata.setProperty(QStringLiteral("description"), scriptAction->description());
            actionMetadata.setProperty(QStringLiteral("requiresProject"), scriptAction->requiresProject());
            actionMetadataArray.setProperty(static_cast<quint32>(index), actionMetadata);
            if (index == actionIndex) {
                currentActionMetadata = actionMetadata;
            }
        }
        metadata.setProperty(QStringLiteral("actions"), actionMetadataArray);

        auto runtime = engine->newObject();
        runtime.setProperty(QStringLiteral("metadata"), metadata);
        runtime.setProperty(QStringLiteral("scriptPath"), script->filePath());
        runtime.setProperty(QStringLiteral("scriptRoot"), script->d_func()->rootPath);
        runtime.setProperty(QStringLiteral("locale"), QLocale().bcp47Name());
        runtime.setProperty(QStringLiteral("platform"), platformName());
        runtime.setProperty(QStringLiteral("kind"), QStringLiteral("action"));
        runtime.setProperty(QStringLiteral("action"), currentActionMetadata);
        runtime.setProperty(QStringLiteral("actionIndex"), static_cast<int>(actionIndex));
        return runtime;
    }

    void RuntimeModuleExtension::abort(ScriptExecutionContext *context, QJSEngine *engine) const {
        if (!context || context->engine() != engine || !context->script() || !context->action()) {
            engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("No script action is currently executing."));
            return;
        }
        m_batchProcessInterface->interruptCurrentExecution();
    }

}

#include "RuntimeModuleExtension.moc"
#include "moc_RuntimeModuleExtension.cpp"
