// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "RuntimeModuleExtension.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJSEngine>
#include <QJSValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocale>
#include <QLoggingCategory>
#include <QPointer>
#include <QStringList>
#include <QtGlobal>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/Script.h>
#include <batchprocess/ScriptAction.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/ScriptMethod.h>
#include <batchprocess/internal/FileSystemPathUtils.h>
#include <batchprocess/private/Script_p.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcRuntimeModuleExtension, "diffscope.batchprocess.runtimemodule.extension")

    namespace {

        constexpr auto RuntimeModuleName = "diffscope:runtime";
        constexpr auto RuntimeModuleResourcePath = ":/diffscope/batchprocess/internalscripts/runtime/runtime-module.js";
        constexpr auto I18nextModuleResourcePath = ":/diffscope/batchprocess/i18next/i18next.mjs";

        QString javaScriptErrorText(const QJSValue &error) {
            auto text = error.toString();
            const auto stack = error.property(QStringLiteral("stack")).toString();
            if (!stack.isEmpty() && !text.contains(stack)) {
                text.append(QLatin1Char('\n'));
                text.append(stack);
            }
            return text;
        }

        QString normalizedLanguageCode(QString code) {
            return code.trimmed().replace(QLatin1Char('_'), QLatin1Char('-')).toLower();
        }

        QString bestLanguageMatch(const QHash<QString, QString> &languages, const QString &requestedCode) {
            if (requestedCode.trimmed().isEmpty()) {
                return {};
            }
            QStringList preferences{requestedCode};
            const QLocale requestedLocale(requestedCode);
            for (const auto &language : requestedLocale.uiLanguages()) {
                preferences.append(language);
            }
            for (const auto &preference : preferences) {
                const auto key = normalizedLanguageCode(preference);
                if (const auto it = languages.constFind(key); it != languages.cend()) {
                    return it.value();
                }
            }
            return {};
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

        Q_INVOKABLE QString currentLocale() const {
            return QLocale().bcp47Name();
        }

        Q_INVOKABLE QString currentPlatform() const {
            return platformName();
        }

        Q_INVOKABLE QString currentScriptIdentity() const {
            const auto script = m_context ? m_context->script() : nullptr;
            if (!script) {
                return {};
            }
            const auto filePath = script->filePath();
            return filePath.isEmpty() ? script->metadata().id() : filePath;
        }

        Q_INVOKABLE QJSValue prepareI18n(const QString &directory, const QString &fallbackLanguage) const;
        Q_INVOKABLE QJSValue readI18nResource(const QString &directory, const QString &language) const;
        Q_INVOKABLE QJSValue createI18nextInstance();

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

    QJSValue RuntimeModuleBridge::prepareI18n(const QString &directory, const QString &fallbackLanguage) const {
        const auto fail = [this](const QString &message) {
            m_engine->throwError(QJSValue::TypeError, message);
            return QJSValue(QJSValue::UndefinedValue);
        };
        const auto script = m_context ? m_context->script() : nullptr;
        if (!script) {
            return fail(BatchProcessInterface::tr("Translation resources are only available while a script is active."));
        }
        const auto rootPath = script->rootPath();
        if (rootPath.isEmpty()) {
            return fail(BatchProcessInterface::tr("The current script has no file-system package."));
        }

        QString resolvedPath;
        QString resolveError;
        if (!resolveFileSystemPath(directory, rootPath, &resolvedPath, &resolveError)) {
            return fail(resolveError);
        }
        const QFileInfo directoryInfo(resolvedPath);
        if (!directoryInfo.exists()) {
            return fail(BatchProcessInterface::tr("The translation directory does not exist:\n%1").arg(QDir::toNativeSeparators(resolvedPath)));
        }
        if (!directoryInfo.isDir()) {
            return fail(BatchProcessInterface::tr("The translation path is not a directory:\n%1").arg(QDir::toNativeSeparators(resolvedPath)));
        }
        const auto canonicalDirectory = normalizedFileSystemPath(directoryInfo.canonicalFilePath());
        if (canonicalDirectory.isEmpty()) {
            return fail(BatchProcessInterface::tr("The translation directory cannot be resolved:\n%1").arg(QDir::toNativeSeparators(resolvedPath)));
        }
        if (!isPathWithin(rootPath, canonicalDirectory)) {
            return fail(BatchProcessInterface::tr("The translation directory resolves outside the script package:\n%1").arg(QDir::toNativeSeparators(canonicalDirectory)));
        }

        QHash<QString, QString> languages;
        const auto entries = QDir(canonicalDirectory).entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &entry : entries) {
            if (entry.suffix().compare(QStringLiteral("json"), Qt::CaseInsensitive) != 0) {
                continue;
            }
            const auto canonicalFile = normalizedFileSystemPath(entry.canonicalFilePath());
            if (canonicalFile.isEmpty() || !isPathWithin(rootPath, canonicalFile)) {
                continue;
            }
            const auto languageCode = entry.completeBaseName();
            const auto key = normalizedLanguageCode(languageCode);
            if (!key.isEmpty() && !languages.contains(key)) {
                languages.insert(key, languageCode);
            }
        }

        QString language = bestLanguageMatch(languages, QLocale().bcp47Name());
        if (language.isEmpty()) {
            language = bestLanguageMatch(languages, fallbackLanguage);
        }

        auto result = m_engine->newObject();
        result.setProperty(QStringLiteral("directory"), canonicalDirectory);
        result.setProperty(QStringLiteral("language"), language);
        return result;
    }

    QJSValue RuntimeModuleBridge::readI18nResource(const QString &directory, const QString &language) const {
        const auto fail = [this](const QString &message) {
            m_engine->throwError(QJSValue::TypeError, message);
            return QJSValue(QJSValue::UndefinedValue);
        };
        const auto script = m_context ? m_context->script() : nullptr;
        if (!script) {
            return fail(BatchProcessInterface::tr("Translation resources are only available while a script is active."));
        }
        const auto rootPath = script->rootPath();
        if (rootPath.isEmpty()) {
            return fail(BatchProcessInterface::tr("The current script has no file-system package."));
        }

        const auto canonicalDirectory = normalizedFileSystemPath(QFileInfo(directory).canonicalFilePath());
        if (canonicalDirectory.isEmpty()) {
            return fail(BatchProcessInterface::tr("The translation directory cannot be resolved:\n%1").arg(QDir::toNativeSeparators(directory)));
        }
        if (!isPathWithin(rootPath, canonicalDirectory)) {
            return fail(BatchProcessInterface::tr("The translation directory resolves outside the script package:\n%1").arg(QDir::toNativeSeparators(canonicalDirectory)));
        }

        const auto key = normalizedLanguageCode(language);
        if (key.isEmpty()) {
            return QJSValue(QJSValue::UndefinedValue);
        }

        QString filePath;
        const auto entries = QDir(canonicalDirectory).entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &entry : entries) {
            if (entry.suffix().compare(QStringLiteral("json"), Qt::CaseInsensitive) != 0 || normalizedLanguageCode(entry.completeBaseName()) != key) {
                continue;
            }
            const auto canonicalFile = normalizedFileSystemPath(entry.canonicalFilePath());
            if (!canonicalFile.isEmpty() && isPathWithin(rootPath, canonicalFile)) {
                filePath = canonicalFile;
            }
            break;
        }
        if (filePath.isEmpty()) {
            return QJSValue(QJSValue::UndefinedValue);
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return fail(BatchProcessInterface::tr("Failed to read the translation file %1.").arg(QDir::toNativeSeparators(filePath)));
        }
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            return fail(BatchProcessInterface::tr("Failed to parse the translation file %1 at offset %L2: %3").arg(QDir::toNativeSeparators(filePath)).arg(parseError.offset).arg(parseError.errorString()));
        }
        if (!document.isObject()) {
            return fail(BatchProcessInterface::tr("The translation file %1 does not contain a JSON object.").arg(QDir::toNativeSeparators(filePath)));
        }
        return m_engine->toScriptValue(document.toVariant());
    }

    QJSValue RuntimeModuleBridge::createI18nextInstance() {
        const auto fail = [this](const QString &message) {
            m_engine->throwError(QJSValue::TypeError, message);
            return QJSValue(QJSValue::UndefinedValue);
        };
        const auto module = m_engine->importModule(QString::fromLatin1(I18nextModuleResourcePath));
        if (m_engine->hasError()) {
            m_engine->throwError(m_engine->catchError());
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (module.isError()) {
            m_engine->throwError(module);
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (!module.isObject()) {
            return fail(BatchProcessInterface::tr("The embedded i18next module did not return a module namespace."));
        }
        const auto createInstance = module.property(QStringLiteral("createInstance"));
        if (!createInstance.isCallable()) {
            return fail(BatchProcessInterface::tr("The embedded i18next module does not export createInstance()."));
        }
        const auto instance = createInstance.call();
        if (m_engine->hasError()) {
            m_engine->throwError(m_engine->catchError());
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (instance.isError()) {
            m_engine->throwError(instance);
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (!instance.isObject()) {
            return fail(BatchProcessInterface::tr("i18next createInstance() did not return an object."));
        }
        return instance;
    }

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
        if (!context || context->engine() != engine || !context->script() || !context->isExecuting()) {
            engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("Runtime script information is only available while a script is executing."));
            return QJSValue(QJSValue::UndefinedValue);
        }

        const auto script = context->script();
        const auto action = context->action();
        const auto actions = script->actions();
        const auto actionIndex = actions.indexOf(action);
        const auto method = context->method();
        const auto methods = script->methods();
        const auto methodIndex = methods.indexOf(method);
        if ((action && actionIndex < 0) || (method && methodIndex < 0)) {
            engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("The current action or method is not part of the current script."));
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

        auto methodMetadataArray = engine->newArray(static_cast<quint32>(methods.size()));
        QJSValue currentMethodMetadata;
        for (qsizetype index = 0; index < methods.size(); ++index) {
            const auto methodMetadata = engine->toScriptValue(methods.at(index)->metadata().toVariantMap());
            methodMetadataArray.setProperty(static_cast<quint32>(index), methodMetadata);
            if (index == methodIndex) {
                currentMethodMetadata = methodMetadata;
            }
        }
        metadata.setProperty(QStringLiteral("methods"), methodMetadataArray);

        auto runtime = engine->newObject();
        runtime.setProperty(QStringLiteral("metadata"), metadata);
        runtime.setProperty(QStringLiteral("isBuiltin"), script->isBuiltin());
        runtime.setProperty(QStringLiteral("scriptPath"), script->filePath());
        runtime.setProperty(QStringLiteral("scriptRoot"), script->rootPath());
        if (action) {
            runtime.setProperty(QStringLiteral("kind"), QStringLiteral("action"));
            runtime.setProperty(QStringLiteral("action"), currentActionMetadata);
            runtime.setProperty(QStringLiteral("actionIndex"), static_cast<int>(actionIndex));
        } else {
            runtime.setProperty(QStringLiteral("kind"), QStringLiteral("method"));
            runtime.setProperty(QStringLiteral("method"), currentMethodMetadata);
            runtime.setProperty(QStringLiteral("methodIndex"), static_cast<int>(methodIndex));
        }
        return runtime;
    }

    void RuntimeModuleExtension::abort(ScriptExecutionContext *context, QJSEngine *engine) const {
        if (!context || context->engine() != engine || !context->script() || !context->isExecuting()) {
            engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("No script is currently executing."));
            return;
        }
        m_batchProcessInterface->interruptCurrentExecution();
    }

}

#include "RuntimeModuleExtension.moc"
#include "moc_RuntimeModuleExtension.cpp"
