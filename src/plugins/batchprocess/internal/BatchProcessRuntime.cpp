// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BatchProcessRuntime.h"

#include <algorithm>
#include <exception>
#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QStringList>
#include <QVariant>
#include <QWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ActionWindowInterfaceBase.h>
#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <batchprocess/JavaScriptConsoleInterface.h>
#include <batchprocess/Script.h>
#include <batchprocess/ScriptAction.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/internal/JavaScriptEngineHelper.h>
#include <batchprocess/private/BatchProcessInterface_p.h>
#include <batchprocess/private/Script_p.h>
#include <batchprocess/private/ScriptAction_p.h>
#include <batchprocess/private/ScriptExecutionContext_p.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcBatchProcessRuntime, "diffscope.batchprocess.runtime")

    namespace {

        QString quoted(const QString &value) {
            return QStringLiteral("\"") + value + QStringLiteral("\"");
        }

        QString callbackExceptionText() {
            return BatchProcessInterface::tr("A registered callback threw an unknown C++ exception.");
        }

        QString builtinScriptFactoryExceptionText() {
            return BatchProcessInterface::tr("A built-in script factory threw an unknown C++ exception.");
        }

        constexpr Qt::CaseSensitivity pathCaseSensitivity() {
#if defined(Q_OS_WIN)
            return Qt::CaseInsensitive;
#else
            return Qt::CaseSensitive;
#endif
        }

        QString normalizedComparisonPath(const QString &path) {
            return QDir::cleanPath(QDir::fromNativeSeparators(path));
        }

        bool pathsEqual(const QString &left, const QString &right) {
            return normalizedComparisonPath(left).compare(normalizedComparisonPath(right), pathCaseSensitivity()) == 0;
        }

        bool isWithinDirectory(const QString &directory, const QString &path) {
            if (pathsEqual(directory, path)) {
                return true;
            }
            auto prefix = normalizedComparisonPath(directory);
            if (!prefix.endsWith(QLatin1Char('/'))) {
                prefix.append(QLatin1Char('/'));
            }
            return normalizedComparisonPath(path).startsWith(prefix, pathCaseSensitivity());
        }

        bool isFileSpecifier(const QString &specifier) {
            return specifier.startsWith(QLatin1Char('.')) || specifier.startsWith(QLatin1Char('/'));
        }

        QString nativeAbsolutePath(const QString &path) {
            return QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
        }

        QList<JavaScriptConsoleStackFrame> consoleStackTrace(const QList<JavaScriptStackFrame> &stackTrace) {
            QList<JavaScriptConsoleStackFrame> result;
            result.reserve(stackTrace.size());
            for (const auto &frame : stackTrace) {
                result.append({frame.functionName, frame.fileUrl, frame.line, frame.column});
            }
            return result;
        }

        const JavaScriptStackFrame *consoleSourceFrame(const QList<JavaScriptStackFrame> &stackTrace) {
            const JavaScriptStackFrame *firstLocalFrame{};
            const JavaScriptStackFrame *firstLocatedFrame{};
            const JavaScriptStackFrame *firstSourceFrame{};
            for (const auto &frame : stackTrace) {
                if (frame.fileUrl.isEmpty()) {
                    continue;
                }
                if (!firstSourceFrame) {
                    firstSourceFrame = &frame;
                }
                if (frame.line > 0 && !firstLocatedFrame) {
                    firstLocatedFrame = &frame;
                }
                if (frame.fileUrl.isLocalFile()) {
                    if (frame.line > 0) {
                        return &frame;
                    }
                    if (!firstLocalFrame) {
                        firstLocalFrame = &frame;
                    }
                }
            }
            return firstLocalFrame ? firstLocalFrame : (firstLocatedFrame ? firstLocatedFrame : firstSourceFrame);
        }

        void appendConsoleError(const QString &message, const QList<JavaScriptStackFrame> &stackTrace) {
            const auto console = JavaScriptConsoleInterface::instance();
            if (!console) {
                return;
            }
            const auto sourceFrame = consoleSourceFrame(stackTrace);
            console->appendMessage(
                JavaScriptConsoleInterface::Error,
                message,
                sourceFrame ? sourceFrame->fileUrl : QUrl{},
                sourceFrame ? sourceFrame->line : -1,
                sourceFrame ? sourceFrame->column : -1,
                consoleStackTrace(stackTrace)
            );
        }

    }

    class RequireBridge : public QObject {
        Q_OBJECT
    public:
        RequireBridge(BatchProcessRuntime *runtime, Script *script, QString rootPath, QString entryPath, QString currentPath, bool singleFile, QObject *parent)
            : QObject(parent), m_runtime(runtime), m_script(script), m_rootPath(std::move(rootPath)), m_entryPath(std::move(entryPath)), m_currentPath(std::move(currentPath)), m_singleFile(singleFile) {
        }

        Q_INVOKABLE QJSValue load(const QString &specifier) {
            return m_runtime->require(m_script.data(), m_rootPath, m_entryPath, m_currentPath, specifier, m_singleFile);
        }

    private:
        BatchProcessRuntime *m_runtime;
        QPointer<Script> m_script;
        QString m_rootPath;
        QString m_entryPath;
        QString m_currentPath;
        bool m_singleFile;
    };

    class BuiltinRequireBridge : public QObject {
        Q_OBJECT
    public:
        explicit BuiltinRequireBridge(BatchProcessRuntime *runtime, QObject *parent)
            : QObject(parent), m_runtime(runtime) {
        }

        Q_INVOKABLE QJSValue load(const QString &specifier) {
            return m_runtime->requireBuiltin(specifier);
        }

    private:
        BatchProcessRuntime *m_runtime;
    };

    class DefineScriptBridge : public QObject {
        Q_OBJECT
    public:
        DefineScriptBridge(BatchProcessRuntime *runtime, Script *script, QObject *parent)
            : QObject(parent), m_runtime(runtime), m_script(script) {
        }

        Q_INVOKABLE QJSValue defineScript(const QJSValue &definition) {
            return m_runtime->defineScript(m_script.data(), definition);
        }

    private:
        BatchProcessRuntime *m_runtime;
        QPointer<Script> m_script;
    };

    struct BatchProcessRuntime::BuiltinScriptRegistration {
        QPointer<QObject> owner;
        BatchProcessInterface::BuiltinScriptFactory factory;
    };

    struct BatchProcessRuntime::ModuleRegistration {
        QString name;
        QPointer<QObject> owner;
        BatchProcessInterface::ModuleFactory factory;
    };

    struct BatchProcessRuntime::GlobalObjectRegistration {
        QString name;
        QPointer<QObject> owner;
        BatchProcessInterface::GlobalObjectFactory factory;
    };

    struct BatchProcessRuntime::EngineExtensionRegistration {
        QPointer<QObject> owner;
        BatchProcessInterface::EngineExtension extension;
    };

    struct BatchProcessRuntime::ActionCallbackRegistration {
        QPointer<QObject> owner;
        BatchProcessInterface::ActionCallback callback;
    };

    struct BatchProcessRuntime::DefinitionRecord {
        struct ActionDefinition {
            QString name;
            QString description;
            bool requiresProject{};
            QJSValue execute;
        };

        QJSValue definition;
        ScriptMetadata metadata;
        QVector<ActionDefinition> actions;
    };

    struct BatchProcessRuntime::ModuleRecord {
        QString moduleId;
        QJSValue moduleObject;
        QJSValue factory;
        QJSValue requireFunction;
        QJSValue defineScriptFunction;
    };

    struct BatchProcessRuntime::ScriptCandidate {
        QString scriptPath;
        QString rootPath;
        QString entryPath;
        QString sortName;
        bool singleFile{};
    };

    struct BatchProcessRuntime::ErrorInfo {
        ErrorInfo() = default;

        explicit ErrorInfo(QString text) : message(text), diagnosticText(std::move(text)) {
        }

        ErrorInfo &operator=(QString text) {
            message = text;
            diagnosticText = std::move(text);
            errorValue = {};
            hasErrorValue = false;
            stackTrace.clear();
            return *this;
        }

        bool isEmpty() const {
            return diagnosticText.isEmpty();
        }

        QString message;
        QString diagnosticText;
        QJSValue errorValue;
        bool hasErrorValue{};
        QList<JavaScriptStackFrame> stackTrace;
    };

    ScriptActionModel::ScriptActionModel(QObject *parent) : QAbstractListModel(parent) {
    }

    int ScriptActionModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_actions.size();
    }

    QVariant ScriptActionModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_actions.size()) {
            return {};
        }
        const auto action = m_actions.at(index.row());
        if (role == SeparatorRole) {
            return !action;
        }
        if (!action) {
            return {};
        }
        switch (role) {
            case NameRole:
                return action->name();
            case DescriptionRole:
                return action->description();
            case ActionRole:
                return QVariant::fromValue(static_cast<QObject *>(action));
            case RequiresProjectRole:
                return action->requiresProject();
            default:
                return {};
        }
    }

    QHash<int, QByteArray> ScriptActionModel::roleNames() const {
        return {
            {NameRole, "name"},
            {DescriptionRole, "description"},
            {ActionRole, "scriptAction"},
            {SeparatorRole, "separator"},
            {RequiresProjectRole, "requiresProject"},
        };
    }

    void ScriptActionModel::setScripts(const QList<Script *> &scripts) {
        beginResetModel();
        m_actions.clear();
        QList<ScriptAction *> builtinActions;
        QList<ScriptAction *> userActions;
        for (const auto script : scripts) {
            if (script->filePath().isEmpty()) {
                builtinActions.append(script->actions());
            } else {
                userActions.append(script->actions());
            }
        }
        m_actions.append(builtinActions);
        if (!builtinActions.isEmpty() && !userActions.isEmpty()) {
            m_actions.append(nullptr);
        }
        m_actions.append(userActions);
        if (!builtinActions.isEmpty() || !userActions.isEmpty()) {
            m_actions.append(nullptr);
        }
        endResetModel();
    }

    BatchProcessRuntime::BatchProcessRuntime(BatchProcessInterfacePrivate *interfacePrivate) : m_interfacePrivate(interfacePrivate) {
        m_builtinScriptRegistrations.reserve(interfacePrivate->builtinScriptRegistrations.size());
        for (const auto &registration : interfacePrivate->builtinScriptRegistrations) {
            m_builtinScriptRegistrations.append({registration.owner, registration.factory});
        }
        m_moduleRegistrations.reserve(interfacePrivate->moduleRegistrations.size());
        for (const auto &registration : interfacePrivate->moduleRegistrations) {
            m_moduleRegistrations.append({registration.name, registration.owner, registration.factory});
        }
        m_globalObjectRegistrations.reserve(interfacePrivate->globalObjectRegistrations.size());
        for (const auto &registration : interfacePrivate->globalObjectRegistrations) {
            m_globalObjectRegistrations.append({registration.name, registration.owner, registration.factory});
        }
        m_engineExtensionRegistrations.reserve(interfacePrivate->engineExtensionRegistrations.size());
        for (const auto &registration : interfacePrivate->engineExtensionRegistrations) {
            m_engineExtensionRegistrations.append({registration.owner, registration.extension});
        }
        m_actionStartedCallbacks.reserve(interfacePrivate->actionStartedCallbacks.size());
        for (const auto &registration : interfacePrivate->actionStartedCallbacks) {
            m_actionStartedCallbacks.append({registration.owner, registration.callback});
        }
        m_actionFinishedCallbacks.reserve(interfacePrivate->actionFinishedCallbacks.size());
        for (const auto &registration : interfacePrivate->actionFinishedCallbacks) {
            m_actionFinishedCallbacks.append({registration.owner, registration.callback});
        }
    }

    BatchProcessRuntime::~BatchProcessRuntime() {
        clearExecutionContext();
        for (const auto script : std::as_const(m_scripts)) {
            for (const auto action : script->actions()) {
                action->d_func()->executeFunction = {};
            }
        }
        m_definitions.clear();
        m_fileModules.clear();
        m_registeredModules.clear();
        m_runtimeHelpers = {};
        qDeleteAll(m_scripts);
        m_scripts.clear();
        m_context.reset();
        m_engine.reset();
    }

    bool BatchProcessRuntime::initialize(const QString &scriptDirectory) {
        qCInfo(lcBatchProcessRuntime) << "Creating JavaScript engine";
        m_engine = std::make_unique<QJSEngine>();
        m_context = std::unique_ptr<ScriptExecutionContext>(new ScriptExecutionContext);
        m_context->d_func()->engine = m_engine.get();

        QFile runtimeFile(QStringLiteral(":/diffscope/batchprocess/internalscripts/runtime/module-context.js"));
        if (!runtimeFile.open(QIODevice::ReadOnly)) {
            qFatal() << "Failed to open the embedded Batch Process module context:" << runtimeFile.errorString();
        }
        m_runtimeHelpers = m_engine->evaluate(QString::fromUtf8(runtimeFile.readAll()), QStringLiteral(":/diffscope/batchprocess/internalscripts/runtime/module-context.js"));
        if (m_runtimeHelpers.isError()) {
            qFatal() << "Failed to evaluate the embedded Batch Process module context:" << errorText(m_runtimeHelpers, QStringLiteral("/module-context.js"));
        }
        if (!m_runtimeHelpers.isObject()) {
            qFatal("The embedded Batch Process module context did not return an object");
        }
        static const QStringList requiredHelpers{
            QStringLiteral("defineReadOnlyGlobal"),
            QStringLiteral("invokeAction"),
            QStringLiteral("invokeModule"),
            QStringLiteral("makeDefineScript"),
            QStringLiteral("makeModule"),
            QStringLiteral("makeRequire"),
        };
        for (const auto &helper : requiredHelpers) {
            if (!m_runtimeHelpers.property(helper).isCallable()) {
                qFatal() << "The embedded Batch Process module context is missing helper:" << helper;
            }
        }

        installBuiltinRequire();
        applyEngineExtensions();

        const auto absoluteDirectory = QFileInfo(scriptDirectory).absoluteFilePath();
        QDir directory(absoluteDirectory);
        if (!directory.exists() && !QDir().mkpath(absoluteDirectory)) {
            const auto message = BatchProcessInterface::tr("Failed to create the script directory:\n%1").arg(nativeAbsolutePath(absoluteDirectory));
            qCCritical(lcBatchProcessRuntime) << message;
            reportLoadError(nativeAbsolutePath(absoluteDirectory), ErrorInfo(message));
            return false;
        }

        const auto rootCanonicalPath = QFileInfo(absoluteDirectory).canonicalFilePath();
        if (rootCanonicalPath.isEmpty()) {
            const auto message = BatchProcessInterface::tr("The script directory cannot be resolved:\n%1").arg(nativeAbsolutePath(absoluteDirectory));
            qCCritical(lcBatchProcessRuntime) << message;
            reportLoadError(nativeAbsolutePath(absoluteDirectory), ErrorInfo(message));
            return false;
        }

        QVector<ScriptCandidate> candidates;
        const auto entries = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
        for (const auto &entry : entries) {
            if (entry.isFile() && entry.suffix().compare(QStringLiteral("js"), Qt::CaseInsensitive) == 0) {
                const auto entryPath = entry.canonicalFilePath();
                if (entryPath.isEmpty() || !isWithinDirectory(rootCanonicalPath, entryPath)) {
                    qCWarning(lcBatchProcessRuntime) << "Ignoring unresolved script candidate" << entry.absoluteFilePath();
                    continue;
                }
                candidates.append({entryPath, rootCanonicalPath, entryPath, entry.fileName(), true});
                qCDebug(lcBatchProcessRuntime) << "Discovered single-file script" << entryPath;
                continue;
            }

            if (entry.isDir()) {
                const QFileInfo indexFile(QDir(entry.absoluteFilePath()).filePath(QStringLiteral("index.js")));
                if (!indexFile.isFile()) {
                    qCWarning(lcBatchProcessRuntime) << "Ignoring script directory without a regular index.js" << entry.absoluteFilePath();
                    continue;
                }
                const auto packagePath = entry.canonicalFilePath();
                const auto entryPath = indexFile.canonicalFilePath();
                if (packagePath.isEmpty() || entryPath.isEmpty() || !isWithinDirectory(packagePath, entryPath) || !isWithinDirectory(rootCanonicalPath, packagePath)) {
                    qCWarning(lcBatchProcessRuntime) << "Ignoring unresolved script package" << entry.absoluteFilePath();
                    continue;
                }
                candidates.append({packagePath, packagePath, entryPath, entry.fileName(), false});
                qCDebug(lcBatchProcessRuntime) << "Discovered multi-file script" << packagePath;
            }
        }

        std::ranges::stable_sort(candidates, [](const ScriptCandidate &left, const ScriptCandidate &right) {
            const auto insensitive = QString::compare(left.sortName, right.sortName, Qt::CaseInsensitive);
            return insensitive == 0 ? left.sortName < right.sortName : insensitive < 0;
        });

        QSet<QString> loadedIds;
        auto failedBuiltinCount = 0;
        const auto builtinScriptRegistrations = m_builtinScriptRegistrations;
        for (const auto &registration : builtinScriptRegistrations) {
            if (registration.owner && registration.factory && !loadBuiltinScript(registration, &loadedIds)) {
                ++failedBuiltinCount;
            }
        }
        qCInfo(lcBatchProcessRuntime) << "Built-in script loading completed" << "registered" << builtinScriptRegistrations.size() << "failed" << failedBuiltinCount;

        auto failedUserCount = 0;
        for (const auto &candidate : std::as_const(candidates)) {
            if (!loadCandidate(candidate, &loadedIds)) {
                ++failedUserCount;
            }
        }
        qCInfo(lcBatchProcessRuntime) << "Script scan completed" << "candidates" << candidates.size() << "loaded" << m_scripts.size() << "failed" << failedUserCount;
        clearExecutionContext();
        return true;
    }

    void BatchProcessRuntime::installBuiltinRequire() {
        auto bridge = new BuiltinRequireBridge(this, m_engine.get());
        const auto requireFunction = m_runtimeHelpers.property(QStringLiteral("makeRequire")).call({m_engine->newQObject(bridge)});
        if (requireFunction.isError()) {
            qFatal() << "Failed to create the built-in Batch Process require function:" << errorText(requireFunction);
        }
        const auto result = m_runtimeHelpers.property(QStringLiteral("defineReadOnlyGlobal")).call({m_engine->globalObject(), QStringLiteral("require"), requireFunction});
        if (result.isError()) {
            qFatal() << "Failed to install the built-in Batch Process require function:" << errorText(result);
        }
    }

    bool BatchProcessRuntime::applyEngineExtensions() {
        const auto globalObject = m_engine->globalObject();
        const auto defineReadOnlyGlobal = m_runtimeHelpers.property(QStringLiteral("defineReadOnlyGlobal"));

        const auto globalObjectRegistrations = m_globalObjectRegistrations;
        for (const auto &registration : globalObjectRegistrations) {
            if (!registration.owner || !registration.factory) {
                continue;
            }
            try {
                const auto value = registration.factory(m_context.get());
                if (m_engine->hasError()) {
                    const auto error = errorInfo(m_engine->catchError());
                    qCCritical(lcBatchProcessRuntime) << "Global object factory failed" << registration.name << error.diagnosticText;
                    reportRuntimeError(registration.name, error);
                    continue;
                }
                if (!registration.owner) {
                    const ErrorInfo error(BatchProcessInterface::tr("The owner was destroyed while the global object factory was running."));
                    qCCritical(lcBatchProcessRuntime) << "Global object owner was destroyed" << registration.name;
                    reportRuntimeError(registration.name, error);
                    continue;
                }
                const auto result = defineReadOnlyGlobal.call({globalObject, registration.name, value});
                if (result.isError()) {
                    const auto error = errorInfo(result);
                    qCCritical(lcBatchProcessRuntime) << "Failed to install global object" << registration.name << error.diagnosticText;
                    reportRuntimeError(registration.name, error);
                    continue;
                }
                qCDebug(lcBatchProcessRuntime) << "Installed global object" << registration.name;
            } catch (const std::exception &error) {
                const ErrorInfo errorInfo(QString::fromLocal8Bit(error.what()));
                qCCritical(lcBatchProcessRuntime) << "Global object factory threw" << registration.name << errorInfo.diagnosticText;
                reportRuntimeError(registration.name, errorInfo);
            } catch (...) {
                const ErrorInfo error(callbackExceptionText());
                qCCritical(lcBatchProcessRuntime) << "Global object factory threw" << registration.name << error.diagnosticText;
                reportRuntimeError(registration.name, error);
            }
        }

        const auto engineExtensionRegistrations = m_engineExtensionRegistrations;
        for (const auto &registration : engineExtensionRegistrations) {
            if (!registration.owner || !registration.extension) {
                continue;
            }
            try {
                registration.extension(m_engine.get(), m_context.get());
                if (m_engine->hasError()) {
                    const auto error = errorInfo(m_engine->catchError());
                    qCCritical(lcBatchProcessRuntime) << "Engine extension failed" << error.diagnosticText;
                    reportRuntimeError(BatchProcessInterface::tr("Engine extension"), error);
                    continue;
                }
                qCDebug(lcBatchProcessRuntime) << "Applied engine extension";
            } catch (const std::exception &error) {
                const ErrorInfo errorInfo(QString::fromLocal8Bit(error.what()));
                qCCritical(lcBatchProcessRuntime) << "Engine extension threw" << errorInfo.diagnosticText;
                reportRuntimeError(BatchProcessInterface::tr("Engine extension"), errorInfo);
            } catch (...) {
                const ErrorInfo error(callbackExceptionText());
                qCCritical(lcBatchProcessRuntime) << "Engine extension threw" << error.diagnosticText;
                reportRuntimeError(BatchProcessInterface::tr("Engine extension"), error);
            }
        }
        return true;
    }

    QJSValue BatchProcessRuntime::require(Script *script, const QString &rootPath, const QString &entryPath, const QString &currentPath, const QString &specifier, bool singleFile) {
        if (!script) {
            m_engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("The script package that created this require() function is no longer active."));
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (!specifier.startsWith(QLatin1Char('/')) && QDir::isAbsolutePath(specifier)) {
            m_engine->throwError(QJSValue::TypeError, BatchProcessInterface::tr("Operating-system absolute module paths are not allowed: %1.").arg(quoted(specifier)));
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (!isFileSpecifier(specifier)) {
            QString errorMessage;
            const auto value = loadRegisteredModule(specifier, &errorMessage);
            if (!errorMessage.isEmpty()) {
                m_engine->throwError(QJSValue::TypeError, errorMessage);
                return QJSValue(QJSValue::UndefinedValue);
            }
            return value;
        }

        QString resolvedPath;
        QString resolveError;
        if (!resolveFile(rootPath, entryPath, currentPath, specifier, singleFile, &resolvedPath, &resolveError)) {
            m_engine->throwError(QJSValue::TypeError, resolveError);
            return QJSValue(QJSValue::UndefinedValue);
        }
        ErrorInfo error;
        const auto value = loadFileModule(script, rootPath, entryPath, resolvedPath, singleFile, &error);
        if (!error.isEmpty()) {
            if (error.hasErrorValue) {
                JavaScriptEngineHelper::throwError(m_engine.get(), error.errorValue, error.stackTrace);
            } else {
                m_engine->throwError(QJSValue::GenericError, error.message);
            }
            return QJSValue(QJSValue::UndefinedValue);
        }
        return value;
    }

    QJSValue BatchProcessRuntime::requireBuiltin(const QString &specifier) {
        const auto script = currentScript();
        if (!script || !script->filePath().isEmpty()) {
            m_engine->throwError(QJSValue::GenericError, BatchProcessInterface::tr("The global require() function is only available while a built-in script is loading or executing."));
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (isFileSpecifier(specifier) || QDir::isAbsolutePath(specifier)) {
            m_engine->throwError(QJSValue::TypeError, BatchProcessInterface::tr("Built-in scripts may only require registered modules: %1.").arg(quoted(specifier)));
            return QJSValue(QJSValue::UndefinedValue);
        }

        QString errorMessage;
        const auto value = loadRegisteredModule(specifier, &errorMessage);
        if (!errorMessage.isEmpty()) {
            m_engine->throwError(QJSValue::TypeError, errorMessage);
            return QJSValue(QJSValue::UndefinedValue);
        }
        return value;
    }

    QJSValue BatchProcessRuntime::loadRegisteredModule(const QString &name, QString *errorMessage) {
        const auto cached = m_registeredModules.constFind(name);
        if (cached != m_registeredModules.cend()) {
            qCDebug(lcBatchProcessRuntime) << "Using cached registered module" << name;
            return cached.value();
        }

        const auto registration = std::ranges::find_if(m_moduleRegistrations, [&name](const ModuleRegistration &item) {
            return item.owner && item.name == name;
        });
        if (registration == m_moduleRegistrations.end()) {
            *errorMessage = BatchProcessInterface::tr("Unknown registered module %1.").arg(quoted(name));
            return {};
        }

        const auto owner = registration->owner;
        const auto factory = registration->factory;
        try {
            const auto value = factory(m_context.get());
            if (m_engine->hasError()) {
                const auto error = m_engine->catchError();
                *errorMessage = BatchProcessInterface::tr("Factory for module %1 failed:\n%2").arg(quoted(name), errorText(error));
                qCCritical(lcBatchProcessRuntime) << "Registered module factory failed" << name << *errorMessage;
                return {};
            }
            if (!owner) {
                *errorMessage = BatchProcessInterface::tr("The owner of module %1 was destroyed while its factory was running.").arg(quoted(name));
                return {};
            }
            m_registeredModules.insert(name, value);
            qCDebug(lcBatchProcessRuntime) << "Created registered module" << name;
            return value;
        } catch (const std::exception &error) {
            *errorMessage = BatchProcessInterface::tr("Factory for module %1 failed:\n%2").arg(quoted(name), QString::fromLocal8Bit(error.what()));
        } catch (...) {
            *errorMessage = BatchProcessInterface::tr("Factory for module %1 failed:\n%2").arg(quoted(name), callbackExceptionText());
        }
        qCCritical(lcBatchProcessRuntime) << "Registered module factory threw" << name << *errorMessage;
        return {};
    }

    bool BatchProcessRuntime::resolveFile(const QString &rootPath, const QString &entryPath, const QString &currentPath, const QString &specifier, bool singleFile, QString *resolvedPath, QString *errorMessage) const {
        QString basePath;
        if (specifier.startsWith(QLatin1Char('/'))) {
            const auto packageRelativePath = specifier.mid(1);
            if (packageRelativePath.startsWith(QLatin1Char('/')) || QDir::isAbsolutePath(packageRelativePath)) {
                *errorMessage = BatchProcessInterface::tr("Operating-system absolute module paths are not allowed: %1.").arg(quoted(specifier));
                return false;
            }
            basePath = QDir(rootPath).filePath(packageRelativePath);
        } else if (specifier.startsWith(QStringLiteral("./")) || specifier.startsWith(QStringLiteral("../"))) {
            basePath = QDir(QFileInfo(currentPath).absolutePath()).filePath(specifier);
        } else {
            *errorMessage = BatchProcessInterface::tr("Invalid file module specifier %1.").arg(quoted(specifier));
            return false;
        }

        QStringList pathsToTry;
        const QFileInfo requestedInfo(basePath);
        if (requestedInfo.suffix().isEmpty()) {
            pathsToTry.append(basePath + QStringLiteral(".js"));
            pathsToTry.append(basePath + QStringLiteral(".json"));
        } else if (requestedInfo.suffix().compare(QStringLiteral("js"), Qt::CaseInsensitive) == 0 || requestedInfo.suffix().compare(QStringLiteral("json"), Qt::CaseInsensitive) == 0) {
            pathsToTry.append(basePath);
        } else {
            *errorMessage = BatchProcessInterface::tr("Unsupported module extension in %1.").arg(quoted(specifier));
            return false;
        }

        for (const auto &path : std::as_const(pathsToTry)) {
            const QFileInfo fileInfo(QDir::cleanPath(path));
            if (!fileInfo.isFile()) {
                continue;
            }
            const auto canonicalPath = fileInfo.canonicalFilePath();
            if (canonicalPath.isEmpty()) {
                continue;
            }
            if (singleFile ? !pathsEqual(canonicalPath, entryPath) : !isWithinDirectory(rootPath, canonicalPath)) {
                *errorMessage = BatchProcessInterface::tr("Module %1 resolves outside the script package.").arg(quoted(specifier));
                return false;
            }
            *resolvedPath = canonicalPath;
            qCDebug(lcBatchProcessRuntime) << "Resolved module" << specifier << "to" << canonicalPath;
            return true;
        }

        *errorMessage = BatchProcessInterface::tr("Cannot find module %1.").arg(quoted(specifier));
        return false;
    }

    QJSValue BatchProcessRuntime::loadFileModule(Script *script, const QString &rootPath, const QString &entryPath, const QString &filePath, bool singleFile, ErrorInfo *errorMessage) {
        auto &scriptModules = m_fileModules[script];
        const auto cached = scriptModules.constFind(filePath);
        if (cached != scriptModules.cend()) {
            qCDebug(lcBatchProcessRuntime) << "Using cached file module" << cached.value()->moduleId;
            return cached.value()->moduleObject.property(QStringLiteral("exports"));
        }

        const auto moduleId = virtualModuleId(rootPath, filePath);
        const auto exportsObject = m_engine->newObject();
        const auto makeModule = m_runtimeHelpers.property(QStringLiteral("makeModule"));
        const auto moduleObject = makeModule.call({moduleId, exportsObject});
        if (moduleObject.isError()) {
            *errorMessage = errorInfo(moduleObject, moduleId);
            return {};
        }

        auto record = std::make_shared<ModuleRecord>();
        record->moduleId = moduleId;
        record->moduleObject = moduleObject;
        scriptModules.insert(filePath, record);

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            *errorMessage = BatchProcessInterface::tr("Failed to read module %1.").arg(quoted(moduleId));
        } else if (QFileInfo(filePath).suffix().compare(QStringLiteral("json"), Qt::CaseInsensitive) == 0) {
            QJsonParseError parseError;
            const auto value = QJsonValue::fromJson(file.readAll(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                *errorMessage = BatchProcessInterface::tr("Failed to parse JSON module %1 at offset %L2: %3").arg(quoted(moduleId)).arg(parseError.offset).arg(parseError.errorString());
            } else {
                record->moduleObject.setProperty(QStringLiteral("exports"), m_engine->toScriptValue(value.toVariant()));
            }
        } else {
            const auto source = QString::fromUtf8(file.readAll());
            record->factory = m_engine->evaluate(QStringLiteral("(function(module, require, defineScript) {") + source + QStringLiteral("\n})"), nativeAbsolutePath(filePath));
            if (record->factory.isError()) {
                *errorMessage = errorInfo(record->factory, moduleId);
            } else {
                auto requireBridge = new RequireBridge(this, script, rootPath, entryPath, filePath, singleFile, m_engine.get());
                auto defineScriptBridge = new DefineScriptBridge(this, script, m_engine.get());
                record->requireFunction = m_runtimeHelpers.property(QStringLiteral("makeRequire")).call({m_engine->newQObject(requireBridge)});
                record->defineScriptFunction = m_runtimeHelpers.property(QStringLiteral("makeDefineScript")).call({m_engine->newQObject(defineScriptBridge)});
                if (record->requireFunction.isError()) {
                    *errorMessage = errorInfo(record->requireFunction, moduleId);
                } else if (record->defineScriptFunction.isError()) {
                    *errorMessage = errorInfo(record->defineScriptFunction, moduleId);
                } else {
                    const auto result = m_runtimeHelpers.property(QStringLiteral("invokeModule")).call({record->factory, record->moduleObject, record->requireFunction, record->defineScriptFunction});
                    if (result.isError()) {
                        *errorMessage = errorInfo(result, moduleId);
                    } else if (!result.property(QStringLiteral("ok")).toBool()) {
                        *errorMessage = errorInfo(result.property(QStringLiteral("error")), moduleId);
                    }
                }
            }
        }

        if (!errorMessage->isEmpty()) {
            qCCritical(lcBatchProcessRuntime) << "Failed to load file module" << moduleId << errorMessage->diagnosticText;
            scriptModules.remove(filePath);
            if (scriptModules.isEmpty()) {
                m_fileModules.remove(script);
            }
            return {};
        }

        qCDebug(lcBatchProcessRuntime) << "Loaded file module" << moduleId;
        return record->moduleObject.property(QStringLiteral("exports"));
    }

    QJSValue BatchProcessRuntime::defineScript(Script *script, const QJSValue &definition) {
        auto throwDefinitionError = [this](const QString &message) {
            m_engine->throwError(QJSValue::TypeError, message);
            return QJSValue(QJSValue::UndefinedValue);
        };
        if (!script) {
            return throwDefinitionError(BatchProcessInterface::tr("The script package that created this defineScript() function is no longer active."));
        }
        auto requiredString = [&throwDefinitionError](const QJSValue &object, const QString &propertyName, bool allowEmpty, QString *result, QJSValue *error) {
            const auto value = object.property(propertyName);
            if (!value.isString()) {
                *error = throwDefinitionError(BatchProcessInterface::tr("Script property %1 must be a string.").arg(quoted(propertyName)));
                return false;
            }
            if (!allowEmpty && value.toString().trimmed().isEmpty()) {
                *error = throwDefinitionError(BatchProcessInterface::tr("Script property %1 must be a non-empty string.").arg(quoted(propertyName)));
                return false;
            }
            *result = value.toString();
            return true;
        };

        if (!definition.isObject() || definition.isNull()) {
            return throwDefinitionError(BatchProcessInterface::tr("defineScript() requires an object."));
        }

        DefinitionRecord record;
        record.definition = definition;
        QJSValue validationError;
        if (!requiredString(definition, QStringLiteral("id"), false, &record.metadata.m_id, &validationError) ||
            !requiredString(definition, QStringLiteral("name"), false, &record.metadata.m_name, &validationError) ||
            !requiredString(definition, QStringLiteral("version"), false, &record.metadata.m_version, &validationError) ||
            !requiredString(definition, QStringLiteral("description"), true, &record.metadata.m_description, &validationError)) {
            return validationError;
        }

        const auto author = definition.property(QStringLiteral("author"));
        if (!author.isUndefined()) {
            if (!author.isString()) {
                return throwDefinitionError(BatchProcessInterface::tr("Script property %1 must be a string when provided.").arg(quoted(QStringLiteral("author"))));
            }
            record.metadata.m_author = author.toString();
        }

        const auto actions = definition.property(QStringLiteral("actions"));
        if (!actions.isArray()) {
            return throwDefinitionError(BatchProcessInterface::tr("Script property %1 must be an array.").arg(quoted(QStringLiteral("actions"))));
        }

        QSet<QString> actionNames;
        const auto actionCount = actions.property(QStringLiteral("length")).toUInt();
        record.actions.reserve(static_cast<qsizetype>(actionCount));
        for (quint32 index = 0; index < actionCount; ++index) {
            const auto action = actions.property(index);
            if (!action.isObject() || action.isNull()) {
                return throwDefinitionError(BatchProcessInterface::tr("Script action %L1 must be an object.").arg(index + 1));
            }

            DefinitionRecord::ActionDefinition actionDefinition;
            if (!requiredString(action, QStringLiteral("name"), false, &actionDefinition.name, &validationError) ||
                !requiredString(action, QStringLiteral("description"), true, &actionDefinition.description, &validationError)) {
                return validationError;
            }
            if (actionNames.contains(actionDefinition.name)) {
                return throwDefinitionError(BatchProcessInterface::tr("Script action name %1 is duplicated.").arg(quoted(actionDefinition.name)));
            }
            actionNames.insert(actionDefinition.name);

            const auto requiresProject = action.property(QStringLiteral("requiresProject"));
            if (!requiresProject.isUndefined() && !requiresProject.isBool()) {
                return throwDefinitionError(BatchProcessInterface::tr("Action property %1 must be a boolean when provided.").arg(quoted(QStringLiteral("requiresProject"))));
            }
            actionDefinition.requiresProject = requiresProject.isBool() && requiresProject.toBool();
            actionDefinition.execute = action.property(QStringLiteral("execute"));
            if (!actionDefinition.execute.isCallable()) {
                return throwDefinitionError(BatchProcessInterface::tr("Action property %1 must be a function.").arg(quoted(QStringLiteral("execute"))));
            }
            record.actions.append(std::move(actionDefinition));
        }

        if (!script->d_func()->metadata.isValid()) {
            script->d_func()->metadata = record.metadata;
        }
        m_definitions[script].append(std::move(record));
        qCDebug(lcBatchProcessRuntime) << "Accepted script definition" << script->d_func()->metadata.id();
        return definition;
    }

    bool BatchProcessRuntime::loadCandidate(const ScriptCandidate &candidate, QSet<QString> *loadedIds) {
        std::unique_ptr<Script> script(new Script(nativeAbsolutePath(candidate.scriptPath), nativeAbsolutePath(candidate.rootPath)));
        setCurrentScript(script.get());

        ErrorInfo error;
        const auto entryExports = loadFileModule(script.get(), candidate.rootPath, candidate.entryPath, candidate.entryPath, candidate.singleFile, &error);
        if (!error.isEmpty()) {
            rejectScript(script.get(), error);
            return false;
        }

        auto definitionsIt = m_definitions.find(script.get());
        if (definitionsIt == m_definitions.end()) {
            error = BatchProcessInterface::tr("The entry module did not call defineScript().");
            rejectScript(script.get(), error);
            return false;
        }
        auto &definitions = definitionsIt.value();

        QJSValue exportedDefinition = entryExports;
        auto selected = std::ranges::find_if(definitions, [&exportedDefinition](const DefinitionRecord &record) {
            return record.definition.strictlyEquals(exportedDefinition);
        });
        if (selected == definitions.end()) {
            exportedDefinition = entryExports.property(QStringLiteral("default"));
            if (m_engine->hasError()) {
                error = errorInfo(m_engine->catchError(), virtualModuleId(candidate.rootPath, candidate.entryPath));
            } else {
                selected = std::ranges::find_if(definitions, [&exportedDefinition](const DefinitionRecord &record) {
                    return record.definition.strictlyEquals(exportedDefinition);
                });
            }
        }

        if (error.isEmpty() && selected == definitions.end()) {
            error = BatchProcessInterface::tr("The entry module must export the object returned by defineScript(), either directly or as module.exports.default.");
        }
        if (!error.isEmpty()) {
            rejectScript(script.get(), error);
            return false;
        }

        return commitScript(std::move(script), *selected, loadedIds);
    }

    bool BatchProcessRuntime::loadBuiltinScript(const BuiltinScriptRegistration &registration, QSet<QString> *loadedIds) {
        std::unique_ptr<Script> script(new Script(QString(), QString()));
        setCurrentScript(script.get());

        ErrorInfo error;
        QJSValue definition;
        const auto owner = registration.owner;
        const auto factory = registration.factory;
        try {
            definition = factory(m_context.get());
            if (m_engine->hasError()) {
                error = errorInfo(m_engine->catchError());
            } else if (definition.isError()) {
                error = errorInfo(definition);
            } else if (!owner) {
                error = BatchProcessInterface::tr("The owner was destroyed while the built-in script factory was running.");
            }
        } catch (const std::exception &exception) {
            error = QString::fromLocal8Bit(exception.what());
        } catch (...) {
            error = builtinScriptFactoryExceptionText();
        }
        if (!error.isEmpty()) {
            rejectScript(script.get(), error);
            return false;
        }

        auto defineScriptBridge = new DefineScriptBridge(this, script.get(), m_engine.get());
        const auto defineScriptFunction = m_runtimeHelpers.property(QStringLiteral("makeDefineScript")).call({m_engine->newQObject(defineScriptBridge)});
        if (m_engine->hasError()) {
            error = errorInfo(m_engine->catchError());
        } else if (defineScriptFunction.isError()) {
            error = errorInfo(defineScriptFunction);
        }

        QJSValue validatedDefinition;
        if (error.isEmpty()) {
            validatedDefinition = defineScriptFunction.call({definition});
            if (m_engine->hasError()) {
                error = errorInfo(m_engine->catchError());
            } else if (validatedDefinition.isError()) {
                error = errorInfo(validatedDefinition);
            }
        }

        auto definitionsIt = m_definitions.find(script.get());
        const DefinitionRecord *selected = nullptr;
        if (definitionsIt != m_definitions.end()) {
            const auto &definitions = std::as_const(definitionsIt.value());
            const auto selectedIt = std::ranges::find_if(definitions, [&validatedDefinition](const DefinitionRecord &record) {
                return record.definition.strictlyEquals(validatedDefinition);
            });
            if (selectedIt != definitions.cend()) {
                selected = &*selectedIt;
            }
        }
        if (error.isEmpty() && !selected) {
            error = BatchProcessInterface::tr("The built-in script factory did not return a valid script definition.");
        }
        if (!error.isEmpty()) {
            rejectScript(script.get(), error);
            return false;
        }

        return commitScript(std::move(script), *selected, loadedIds);
    }

    bool BatchProcessRuntime::commitScript(std::unique_ptr<Script> script, const DefinitionRecord &definition, QSet<QString> *loadedIds) {
        if (loadedIds->contains(definition.metadata.id())) {
            rejectScript(script.get(), ErrorInfo(BatchProcessInterface::tr("Script ID %1 is already loaded.").arg(quoted(definition.metadata.id()))));
            return false;
        }

        script->d_func()->metadata = definition.metadata;
        for (const auto &actionDefinition : definition.actions) {
            auto action = new ScriptAction(script.get(), script.get());
            action->d_func()->name = actionDefinition.name;
            action->d_func()->description = actionDefinition.description;
            action->d_func()->requiresProject = actionDefinition.requiresProject;
            action->d_func()->executeFunction = actionDefinition.execute;
            script->d_func()->actions.append(action);
        }

        loadedIds->insert(definition.metadata.id());
        qCInfo(lcBatchProcessRuntime) << "Loaded script" << definition.metadata.id() << script->filePath() << "actions" << script->actions().size();
        m_definitions.remove(script.get());
        m_scripts.append(script.release());
        clearExecutionContext();
        return true;
    }

    void BatchProcessRuntime::setCurrentScript(Script *script) {
        auto contextPrivate = m_context->d_func();
        contextPrivate->script = script;
        contextPrivate->action = nullptr;
        contextPrivate->windowInterface = nullptr;
        contextPrivate->projectWindowInterface = nullptr;
        contextPrivate->projectDocumentContext = nullptr;
    }

    void BatchProcessRuntime::rejectScript(Script *script, const ErrorInfo &error) {
        qCCritical(lcBatchProcessRuntime) << (script->filePath().isEmpty() ? "Rejected built-in script" : "Rejected script candidate") << script->filePath() << error.diagnosticText;
        reportLoadError(script->filePath(), error);
        purgeScriptModules(script);
        m_definitions.remove(script);
        clearExecutionContext();
    }

    void BatchProcessRuntime::purgeScriptModules(Script *script) {
        m_fileModules.remove(script);
    }

    bool BatchProcessRuntime::executeAction(ScriptAction *action, Core::ActionWindowInterfaceBase *windowInterface) {
        if (m_executing || currentScript()) {
            qCWarning(lcBatchProcessRuntime) << "Rejected reentrant action execution";
            return false;
        }

        const auto script = action ? action->script() : nullptr;
        if (!script || !m_scripts.contains(script) || !script->actions().contains(action)) {
            qCWarning(lcBatchProcessRuntime) << "Rejected action from an inactive runtime";
            return false;
        }

        auto projectWindowInterface = qobject_cast<Core::ProjectWindowInterface *>(windowInterface);
        if (action->requiresProject() && !projectWindowInterface) {
            qCWarning(lcBatchProcessRuntime) << "Action requires a project" << script->metadata().id() << action->name();
            SVS::MessageBox::warning(
                Core::RuntimeInterface::qmlEngine(),
                windowInterface ? windowInterface->window() : nullptr,
                BatchProcessInterface::tr("Project Required"),
                BatchProcessInterface::tr("The action %1 requires an open project.").arg(quoted(action->name()))
            );
            return false;
        }

        m_engine->setInterrupted(false);
        m_executing = true;
        auto contextPrivate = m_context->d_func();
        contextPrivate->script = script;
        contextPrivate->action = action;
        contextPrivate->windowInterface = windowInterface;
        contextPrivate->projectWindowInterface = projectWindowInterface;
        contextPrivate->projectDocumentContext = projectWindowInterface ? projectWindowInterface->projectDocumentContext() : nullptr;

        qCInfo(lcBatchProcessRuntime) << "Starting script action" << script->metadata().id() << action->name();
        const auto actionStartedCallbacks = m_actionStartedCallbacks;
        for (const auto &registration : actionStartedCallbacks) {
            if (!registration.owner || !registration.callback) {
                continue;
            }
            try {
                registration.callback(m_context.get());
                if (m_engine->hasError()) {
                    const auto message = errorText(m_engine->catchError());
                    qCCritical(lcBatchProcessRuntime) << "Action-start callback failed" << message;
                }
            } catch (const std::exception &error) {
                qCCritical(lcBatchProcessRuntime) << "Action-start callback threw" << error.what();
            } catch (...) {
                qCCritical(lcBatchProcessRuntime) << "Action-start callback threw" << callbackExceptionText();
            }
        }

        bool succeeded = true;
        ErrorInfo error;
        m_interfacePrivate->beginExecution(m_engine.get());
        const auto result = m_runtimeHelpers.property(QStringLiteral("invokeAction")).call({action->executeFunction()});
        m_interfacePrivate->endExecution(m_engine.get());
        if (result.isError()) {
            succeeded = false;
            error = errorInfo(result);
        } else if (!result.property(QStringLiteral("ok")).toBool()) {
            succeeded = false;
            error = errorInfo(result.property(QStringLiteral("error")));
        } else {
            const auto returnValue = result.property(QStringLiteral("value"));
            if (returnValue.isObject() || returnValue.isCallable()) {
                const auto then = returnValue.property(QStringLiteral("then"));
                if (m_engine->hasError()) {
                    succeeded = false;
                    error = errorInfo(m_engine->catchError());
                } else if (then.isCallable()) {
                    succeeded = false;
                    error = BatchProcessInterface::tr("The action returned a Promise or another thenable. Batch Process actions must complete synchronously.");
                }
            }
        }

        const auto actionFinishedCallbacks = m_actionFinishedCallbacks;
        for (const auto &registration : actionFinishedCallbacks) {
            if (!registration.owner || !registration.callback) {
                continue;
            }
            try {
                registration.callback(m_context.get());
                if (m_engine->hasError()) {
                    const auto message = errorText(m_engine->catchError());
                    qCCritical(lcBatchProcessRuntime) << "Action-finished callback failed" << message;
                }
            } catch (const std::exception &error) {
                qCCritical(lcBatchProcessRuntime) << "Action-finished callback threw" << error.what();
            } catch (...) {
                qCCritical(lcBatchProcessRuntime) << "Action-finished callback threw" << callbackExceptionText();
            }
        }

        if (!succeeded) {
            qCCritical(lcBatchProcessRuntime) << "Script action failed" << script->metadata().id() << action->name() << error.diagnosticText;
            reportExecutionError(action, windowInterface, error);
        }
        qCInfo(lcBatchProcessRuntime) << "Finished script action" << script->metadata().id() << action->name() << "success" << succeeded;

        m_engine->setInterrupted(false);
        clearExecutionContext();
        m_executing = false;
        return succeeded;
    }

    void BatchProcessRuntime::removeOwner(QObject *owner) {
        auto belongsToOwner = [owner](const auto &registration) {
            return !registration.owner || registration.owner.data() == owner;
        };
        erase_if(m_builtinScriptRegistrations, belongsToOwner);
        erase_if(m_moduleRegistrations, belongsToOwner);
        erase_if(m_globalObjectRegistrations, belongsToOwner);
        erase_if(m_engineExtensionRegistrations, belongsToOwner);
        erase_if(m_actionStartedCallbacks, belongsToOwner);
        erase_if(m_actionFinishedCallbacks, belongsToOwner);

        for (auto it = m_registeredModules.begin(); it != m_registeredModules.end();) {
            const auto stillRegistered = std::ranges::any_of(m_moduleRegistrations, [&it](const ModuleRegistration &registration) {
                return registration.name == it.key();
            });
            if (!stillRegistered) {
                it = m_registeredModules.erase(it);
            } else {
                ++it;
            }
        }
    }

    QList<Script *> BatchProcessRuntime::scripts() const {
        return m_scripts;
    }

    Script *BatchProcessRuntime::currentScript() const {
        return m_context ? m_context->script() : nullptr;
    }

    ScriptExecutionContext *BatchProcessRuntime::executionContext() const {
        return m_context.get();
    }

    void BatchProcessRuntime::reportLoadError(const QString &scriptPath, const ErrorInfo &error) const {
        appendConsoleError(error.message, error.stackTrace);
        if (scriptPath.isEmpty()) {
            Core::CoreInterface::sendNotification(
                SVS::SVSCraft::Critical,
                BatchProcessInterface::tr("Batch Process"),
                BatchProcessInterface::tr("Failed to load a built-in script:\n\n%1").arg(error.diagnosticText)
            );
            return;
        }
        Core::CoreInterface::sendNotification(
            SVS::SVSCraft::Critical,
            BatchProcessInterface::tr("Batch Process"),
            BatchProcessInterface::tr("Failed to load script %1:\n\n%2").arg(quoted(QDir::toNativeSeparators(scriptPath)), error.diagnosticText)
        );
    }

    void BatchProcessRuntime::reportRuntimeError(const QString &component, const ErrorInfo &error) const {
        appendConsoleError(error.message, error.stackTrace);
        Core::CoreInterface::sendNotification(
            SVS::SVSCraft::Critical,
            BatchProcessInterface::tr("Batch Process"),
            BatchProcessInterface::tr("Failed to initialize %1:\n\n%2").arg(quoted(component), error.diagnosticText)
        );
    }

    void BatchProcessRuntime::reportExecutionError(ScriptAction *action, Core::ActionWindowInterfaceBase *windowInterface, const ErrorInfo &error) const {
        appendConsoleError(error.message, error.stackTrace);
        SVS::MessageBox::critical(
            Core::RuntimeInterface::qmlEngine(),
            windowInterface ? windowInterface->window() : nullptr,
            BatchProcessInterface::tr("Batch Process"),
            BatchProcessInterface::tr("Script %1, action %2 failed:\n\n%3").arg(quoted(action->script()->metadata().name()), quoted(action->name()), error.diagnosticText)
        );
    }

    BatchProcessRuntime::ErrorInfo BatchProcessRuntime::errorInfo(const QJSValue &error, const QString &moduleId) const {
        auto javaScriptStackTrace = JavaScriptEngineHelper::stackTrace(error);
        const auto exceptionStackTrace = JavaScriptEngineHelper::takeExceptionStackTrace(m_engine.get());
        if (javaScriptStackTrace.isEmpty()) {
            javaScriptStackTrace = exceptionStackTrace;
        }

        QString message;
        QString fileName;
        QString stack;
        auto lineNumber = 0;

        if (error.isError()) {
            const auto name = error.property(QStringLiteral("name")).toString();
            message = error.property(QStringLiteral("message")).toString();
            if (!name.isEmpty() && name != QStringLiteral("Error")) {
                message = message.isEmpty() ? name : QStringLiteral("%1: %2").arg(name, message);
            }
            fileName = error.property(QStringLiteral("fileName")).toString();
            lineNumber = error.property(QStringLiteral("lineNumber")).toInt();
            stack = error.property(QStringLiteral("stack")).toString();
        } else if (error.isString() || error.isNumber() || error.isBool() || error.isNull() || error.isUndefined()) {
            message = error.toString();
        } else if (error.isCallable()) {
            message = BatchProcessInterface::tr("The script threw a function value.");
        } else if (error.isObject()) {
            message = BatchProcessInterface::tr("The script threw a non-Error object.");
        } else {
            message = error.toString();
        }
        if (message.isEmpty()) {
            message = BatchProcessInterface::tr("The script threw an unidentified value.");
        }
        const auto hasErrorFileName = !fileName.isEmpty();
        if (fileName.isEmpty()) {
            fileName = moduleId;
        }

        QString location;
        if (!fileName.isEmpty()) {
            location = lineNumber > 0 ? QStringLiteral("%1:%2").arg(fileName).arg(lineNumber) : fileName;
        }

        QStringList parts;
        parts.append(message);
        if (!location.isEmpty() && !message.contains(location)) {
            parts.append(BatchProcessInterface::tr("Location: %1").arg(location));
        }
        if (!stack.isEmpty() && stack != message) {
            parts.append(stack);
        }

        ErrorInfo result;
        result.message = message;
        result.diagnosticText = parts.join(QLatin1Char('\n'));
        result.errorValue = error;
        result.hasErrorValue = true;
        result.stackTrace = std::move(javaScriptStackTrace);
        if (!fileName.isEmpty()) {
            QUrl fileUrl;
            if (hasErrorFileName && QDir::isAbsolutePath(fileName)) {
                fileUrl = QUrl::fromLocalFile(QFileInfo(fileName).absoluteFilePath());
            } else {
                fileUrl = QUrl(fileName);
            }
            auto matchingFrame = std::ranges::find_if(result.stackTrace, [&fileUrl](const JavaScriptStackFrame &frame) {
                if (frame.fileUrl.isLocalFile() && fileUrl.isLocalFile()) {
                    return pathsEqual(frame.fileUrl.toLocalFile(), fileUrl.toLocalFile());
                }
                return frame.fileUrl == fileUrl;
            });
            if (matchingFrame != result.stackTrace.end()) {
                if (matchingFrame->line < 1 && lineNumber > 0) {
                    matchingFrame->line = lineNumber;
                }
            } else if (result.stackTrace.isEmpty() || hasErrorFileName) {
                result.stackTrace.prepend({{}, fileUrl, lineNumber, -1});
            }
        }
        return result;
    }

    QString BatchProcessRuntime::errorText(const QJSValue &error, const QString &moduleId) const {
        return errorInfo(error, moduleId).diagnosticText;
    }

    QString BatchProcessRuntime::virtualModuleId(const QString &rootPath, const QString &filePath) const {
        auto relativePath = QDir(rootPath).relativeFilePath(filePath);
        relativePath = QDir::fromNativeSeparators(relativePath);
        return QLatin1Char('/') + relativePath;
    }

    void BatchProcessRuntime::clearExecutionContext() {
        if (!m_context) {
            return;
        }
        auto contextPrivate = m_context->d_func();
        contextPrivate->script = nullptr;
        contextPrivate->action = nullptr;
        contextPrivate->windowInterface = nullptr;
        contextPrivate->projectWindowInterface = nullptr;
        contextPrivate->projectDocumentContext = nullptr;
    }

}

#include "BatchProcessRuntime.moc"
#include "moc_BatchProcessRuntime.cpp"
