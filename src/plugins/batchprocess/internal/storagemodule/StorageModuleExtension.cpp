// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "StorageModuleExtension.h"

#include <limits>
#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QSaveFile>

#include <CoreApi/applicationinfo.h>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/Storage.h>
#include <batchprocess/StorageInterface.h>
#include <batchprocess/private/StorageInterface_p.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcStorageModule, "diffscope.batchprocess.storagemodule")

    namespace {

        constexpr auto ModuleName = "diffscope:storage";
        constexpr auto ModuleResourcePath = ":/diffscope/batchprocess/internalscripts/storage/storage-module.js";
        constexpr auto ModuleSourceUrl = "qrc:/diffscope/batchprocess/internalscripts/storage/storage-module.js";

        QString javaScriptErrorText(const QJSValue &error) {
            auto text = error.toString();
            const auto stack = error.property(QStringLiteral("stack")).toString();
            if (!stack.isEmpty() && !text.contains(stack)) {
                text.append(QLatin1Char('\n'));
                text.append(stack);
            }
            return text;
        }

        QString errorCodeName(StorageErrorCode code) {
            switch (code) {
                case StorageErrorCode::None:
                    return QStringLiteral("unknown");
                case StorageErrorCode::InvalidState:
                    return QStringLiteral("invalidState");
                case StorageErrorCode::Io:
                    return QStringLiteral("io");
                case StorageErrorCode::Unknown:
                    return QStringLiteral("unknown");
            }
            return QStringLiteral("unknown");
        }

    }

    class StorageModuleBridge;

    class StorageBridge : public QObject {
        Q_OBJECT
    public:
        StorageBridge(StorageModuleBridge *moduleBridge, Storage *storage, QObject *parent);

        Q_INVOKABLE QJSValue id() const;
        Q_INVOKABLE QJSValue length() const;
        Q_INVOKABLE QJSValue key(double index) const;
        Q_INVOKABLE QJSValue getItem(const QString &key) const;
        Q_INVOKABLE QJSValue setItem(const QString &key, const QString &value) const;
        Q_INVOKABLE QJSValue removeItem(const QString &key) const;
        Q_INVOKABLE QJSValue clear() const;

        Storage *storage() const;
        QString storageId() const;
        ScriptExecutionContext *context() const;

    private:
        QPointer<StorageModuleBridge> m_moduleBridge;
        QPointer<Storage> m_storage;
        QString m_id;
    };

    class StorageModuleBridge : public QObject {
        Q_OBJECT
    public:
        struct WrappedStorage {
            QPointer<StorageBridge> bridge;
            QJSValue rawValue;
            QJSValue wrappedValue;
        };

        StorageModuleBridge(StorageModuleExtension *extension, ScriptExecutionContext *context, QJSEngine *engine)
            : QObject(engine), m_extension(extension), m_context(context), m_engine(engine) {
        }

        ScriptExecutionContext *context() const {
            return m_context.data();
        }

        QJSValue success(const QJSValue &value = QJSValue(QJSValue::UndefinedValue)) const {
            auto result = m_engine->newObject();
            result.setProperty(QStringLiteral("ok"), true);
            result.setProperty(QStringLiteral("value"), value);
            return result;
        }

        QJSValue failure(const StorageError &error) const {
            qCWarning(lcStorageModule) << "Storage operation failed" << error.operation << errorCodeName(error.code) << error.storageId << error.key.value_or(QString()) << error.message;
            auto errorObject = m_engine->newObject();
            errorObject.setProperty(QStringLiteral("code"), errorCodeName(error.code));
            errorObject.setProperty(QStringLiteral("message"), error.message);
            errorObject.setProperty(QStringLiteral("operation"), error.operation);
            errorObject.setProperty(QStringLiteral("storageId"), error.storageId);
            errorObject.setProperty(QStringLiteral("key"), error.key ? QJSValue(*error.key) : QJSValue(QJSValue::NullValue));
            auto result = m_engine->newObject();
            result.setProperty(QStringLiteral("ok"), false);
            result.setProperty(QStringLiteral("error"), errorObject);
            return result;
        }

        QJSValue invalidStorage(const QString &operation, const QString &id = {}, const std::optional<QString> &key = std::nullopt) const {
            StorageError error;
            error.code = StorageErrorCode::InvalidState;
            error.operation = operation;
            error.storageId = id;
            error.key = key;
            error.message = StorageInterface::tr("The storage is no longer available.");
            return failure(error);
        }

        QJSValue rawStorage(Storage *storage) {
            if (!storage) {
                return QJSValue(QJSValue::UndefinedValue);
            }
            auto it = m_wrappedStorages.find(storage);
            if (it != m_wrappedStorages.end() && it->bridge && it->rawValue.isObject()) {
                return it->rawValue;
            }

            auto bridge = new StorageBridge(this, storage, m_engine);
            const auto rawValue = m_engine->newQObject(bridge);
            WrappedStorage wrappedStorage;
            wrappedStorage.bridge = bridge;
            wrappedStorage.rawValue = rawValue;
            m_wrappedStorages.insert(storage, wrappedStorage);
            connect(storage, &QObject::destroyed, this, [this, storage] {
                m_wrappedStorages.remove(storage);
            });
            return rawValue;
        }

        QJSValue wrappedStorage(Storage *storage, const QJSValue &wrap, StorageError *error) {
            if (!storage || !wrap.isCallable()) {
                StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("wrap"), storage ? storage->id() : QString(), StorageInterface::tr("The JavaScript storage wrapper is not available."));
                return QJSValue(QJSValue::UndefinedValue);
            }
            const auto rawValue = rawStorage(storage);
            auto it = m_wrappedStorages.find(storage);
            if (it != m_wrappedStorages.end() && it->wrappedValue.isObject()) {
                return it->wrappedValue;
            }
            const auto value = wrap.call({rawValue});
            if (value.isError() || m_engine->hasError() || !value.isObject()) {
                const auto jsError = m_engine->hasError() ? m_engine->catchError() : value;
                StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("wrap"), storage->id(), javaScriptErrorText(jsError));
                return QJSValue(QJSValue::UndefinedValue);
            }
            it = m_wrappedStorages.find(storage);
            if (it != m_wrappedStorages.end()) {
                it->wrappedValue = value;
            }
            if (error) {
                error->clear();
            }
            return value;
        }

        Q_INVOKABLE QJSValue session(const QString &id) {
            StorageError error;
            const auto storage = m_extension ? m_extension->sessionStorage(id, &error) : nullptr;
            if (!storage) {
                return failure(error);
            }
            return success(rawStorage(storage));
        }

        Q_INVOKABLE QJSValue persistent(const QString &id) {
            StorageError error;
            const auto storage = m_extension ? m_extension->persistentStorage(id, &error) : nullptr;
            if (!storage) {
                return failure(error);
            }
            return success(rawStorage(storage));
        }

    private:
        QPointer<StorageModuleExtension> m_extension;
        QPointer<ScriptExecutionContext> m_context;
        QJSEngine *m_engine;
        QHash<Storage *, WrappedStorage> m_wrappedStorages;
    };

    StorageBridge::StorageBridge(StorageModuleBridge *moduleBridge, Storage *storage, QObject *parent)
        : QObject(parent), m_moduleBridge(moduleBridge), m_storage(storage), m_id(storage ? storage->id() : QString()) {
    }

    QJSValue StorageBridge::id() const {
        return m_moduleBridge && m_storage ? m_moduleBridge->success(QJSValue(m_storage->id())) : m_moduleBridge ? m_moduleBridge->invalidStorage(QStringLiteral("id"), m_id) : QJSValue(QJSValue::UndefinedValue);
    }

    QJSValue StorageBridge::length() const {
        return m_moduleBridge && m_storage ? m_moduleBridge->success(QJSValue(static_cast<double>(m_storage->length()))) : m_moduleBridge ? m_moduleBridge->invalidStorage(QStringLiteral("length"), m_id) : QJSValue(QJSValue::UndefinedValue);
    }

    QJSValue StorageBridge::key(double index) const {
        if (!m_moduleBridge) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (!m_storage) {
            return m_moduleBridge->invalidStorage(QStringLiteral("key"), m_id);
        }
        if (index > static_cast<double>(std::numeric_limits<qsizetype>::max())) {
            return m_moduleBridge->success(QJSValue(QJSValue::NullValue));
        }
        const auto value = m_storage->key(static_cast<qsizetype>(index));
        return m_moduleBridge->success(value ? QJSValue(*value) : QJSValue(QJSValue::NullValue));
    }

    QJSValue StorageBridge::getItem(const QString &key) const {
        if (!m_moduleBridge) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (!m_storage) {
            return m_moduleBridge->invalidStorage(QStringLiteral("getItem"), m_id, key);
        }
        const auto value = m_storage->getItem(key);
        return m_moduleBridge->success(value ? QJSValue(*value) : QJSValue(QJSValue::NullValue));
    }

    QJSValue StorageBridge::setItem(const QString &key, const QString &value) const {
        if (!m_moduleBridge) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (!m_storage) {
            return m_moduleBridge->invalidStorage(QStringLiteral("setItem"), m_id, key);
        }
        StorageError error;
        if (!m_storage->setItem(key, value, &error)) {
            return m_moduleBridge->failure(error);
        }
        return m_moduleBridge->success();
    }

    QJSValue StorageBridge::removeItem(const QString &key) const {
        if (!m_moduleBridge) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (!m_storage) {
            return m_moduleBridge->invalidStorage(QStringLiteral("removeItem"), m_id, key);
        }
        StorageError error;
        if (!m_storage->removeItem(key, &error)) {
            return m_moduleBridge->failure(error);
        }
        return m_moduleBridge->success();
    }

    QJSValue StorageBridge::clear() const {
        if (!m_moduleBridge) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        if (!m_storage) {
            return m_moduleBridge->invalidStorage(QStringLiteral("clear"), m_id);
        }
        StorageError error;
        if (!m_storage->clear(&error)) {
            return m_moduleBridge->failure(error);
        }
        return m_moduleBridge->success();
    }

    Storage *StorageBridge::storage() const {
        return m_storage.data();
    }

    QString StorageBridge::storageId() const {
        return m_id;
    }

    ScriptExecutionContext *StorageBridge::context() const {
        return m_moduleBridge ? m_moduleBridge->context() : nullptr;
    }

    StorageModuleExtension::StorageModuleExtension(BatchProcessInterface *batchProcessInterface, StorageInterface *storageInterface, QObject *parent)
        : QObject(parent), m_batchProcessInterface(batchProcessInterface), m_storageInterface(storageInterface) {
        Q_ASSERT(m_batchProcessInterface);
        Q_ASSERT(m_storageInterface);
        m_storageInterface->d_func()->moduleExtension = this;
        m_persistentFilePath = QDir(Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::RuntimeData))
                                   .filePath(QCoreApplication::applicationName() + QStringLiteral(".scriptstorage.json"));
        loadPersistentStorage();

        const auto extensionRegistered = m_batchProcessInterface->registerEngineExtension(this, [this](QJSEngine *engine, ScriptExecutionContext *context) {
            installIntoEngine(engine, context);
        });
        const auto moduleRegistered = m_batchProcessInterface->registerModule(QString::fromLatin1(ModuleName), this, [this](ScriptExecutionContext *context) {
            return createModule(context);
        });
        if (!extensionRegistered || !moduleRegistered) {
            qFatal("Failed to register the Batch Process storage module");
        }
        qCDebug(lcStorageModule) << "Registered storage module" << ModuleName;
    }

    StorageModuleExtension::~StorageModuleExtension() = default;

    void StorageModuleExtension::installIntoEngine(QJSEngine *engine, ScriptExecutionContext *context) {
        if (!engine || !context || context->engine() != engine) {
            qFatal("The Batch Process storage engine extension received an invalid execution context");
        }
        QFile moduleFile(QString::fromLatin1(ModuleResourcePath));
        if (!moduleFile.open(QIODevice::ReadOnly)) {
            qFatal() << "Failed to open the embedded Batch Process storage module:" << moduleFile.errorString();
        }
        const auto factory = engine->evaluate(QString::fromUtf8(moduleFile.readAll()), QString::fromLatin1(ModuleSourceUrl));
        if (factory.isError()) {
            qFatal() << "Failed to evaluate the embedded Batch Process storage module:" << javaScriptErrorText(factory);
        }
        if (!factory.isCallable()) {
            qFatal("The embedded Batch Process storage module did not return a factory function");
        }
        auto bridge = new StorageModuleBridge(this, context, engine);
        const auto bundle = factory.call({engine->newQObject(bridge)});
        if (engine->hasError()) {
            qFatal() << "Failed to create the Batch Process storage module:" << javaScriptErrorText(engine->catchError());
        }
        if (bundle.isError()) {
            qFatal() << "Failed to create the Batch Process storage module:" << javaScriptErrorText(bundle);
        }
        if (!bundle.isObject() || !bundle.property(QStringLiteral("exports")).isObject() || !bundle.property(QStringLiteral("wrapStorage")).isCallable() || !bundle.property(QStringLiteral("unwrapStorage")).isCallable() || !bundle.property(QStringLiteral("bridge")).isObject()) {
            qFatal("The embedded Batch Process storage module factory returned an invalid helper bundle");
        }
        m_engineBundles.insert(engine, bundle);
        connect(context, &QObject::destroyed, this, [this, engine] {
            m_engineBundles.remove(engine);
        });
    }

    QJSValue StorageModuleExtension::createModule(ScriptExecutionContext *context) const {
        StorageError error;
        const auto exports = helper(context, QStringLiteral("exports"), &error);
        if (!exports.isObject()) {
            qFatal() << "The embedded Batch Process storage module exports are unavailable:" << error.message;
        }
        return exports;
    }

    QJSValue StorageModuleExtension::helper(ScriptExecutionContext *context, const QString &name, StorageError *error) const {
        if (!context || !context->engine()) {
            StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("interop"), {}, StorageInterface::tr("The JavaScript execution context is not available."));
            return QJSValue(QJSValue::UndefinedValue);
        }
        const auto bundle = m_engineBundles.value(context->engine());
        if (!bundle.isObject() || !bundle.hasProperty(name)) {
            StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("interop"), {}, StorageInterface::tr("The JavaScript storage module is not installed in this engine."));
            return QJSValue(QJSValue::UndefinedValue);
        }
        return bundle.property(name);
    }

    QJSValue StorageModuleExtension::wrapStorage(ScriptExecutionContext *context, Storage *storage, StorageError *error) const {
        const auto wrap = helper(context, QStringLiteral("wrapStorage"), error);
        const auto moduleBridge = qobject_cast<StorageModuleBridge *>(helper(context, QStringLiteral("bridge"), error).toQObject());
        if (!wrap.isCallable() || !moduleBridge || moduleBridge->context() != context) {
            StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("wrap"), storage ? storage->id() : QString(), StorageInterface::tr("The JavaScript storage bridge is not available."));
            return QJSValue(QJSValue::UndefinedValue);
        }
        return moduleBridge->wrappedStorage(storage, wrap, error);
    }

    bool StorageModuleExtension::unwrapStorage(ScriptExecutionContext *context, const QJSValue &value, Storage **storage, StorageError *error) const {
        const auto unwrap = helper(context, QStringLiteral("unwrapStorage"), error);
        if (!unwrap.isCallable()) {
            return false;
        }
        auto engine = context->engine();
        const auto result = unwrap.call({value});
        if (result.isError() || engine->hasError()) {
            if (engine->hasError()) {
                engine->catchError();
            }
            StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("unwrap"), {}, StorageInterface::tr("The value is not a storage from the current JavaScript engine."));
            return false;
        }
        const auto bridge = qobject_cast<StorageBridge *>(result.toQObject());
        const auto resolvedStorage = bridge && bridge->context() == context ? bridge->storage() : nullptr;
        if (!resolvedStorage) {
            StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("unwrap"), bridge ? bridge->storageId() : QString(), StorageInterface::tr("The value is not a live storage from the current JavaScript engine."));
            return false;
        }
        *storage = resolvedStorage;
        if (error) {
            error->clear();
        }
        return true;
    }

    Storage *StorageModuleExtension::sessionStorage(const QString &id, StorageError *error) {
        const auto existing = m_sessionStorages.value(id);
        if (existing) {
            if (error) {
                error->clear();
            }
            return existing;
        }
        auto storage = m_storageInterface->createStorage(id, {}, this);
        m_sessionStorages.insert(id, storage);
        qCDebug(lcStorageModule) << "Created session storage bucket" << id;
        if (error) {
            error->clear();
        }
        return storage;
    }

    Storage *StorageModuleExtension::persistentStorage(const QString &id, StorageError *error) {
        const auto existing = m_persistentStorages.value(id);
        if (existing) {
            if (error) {
                error->clear();
            }
            return existing;
        }
        auto storage = m_storageInterface->createStorage(id, m_persistentItems.value(id), this, [this, id](const StorageItems &items, StorageError *commitError) {
            return commitPersistentStorage(id, items, commitError);
        });
        m_persistentStorages.insert(id, storage);
        qCDebug(lcStorageModule) << "Opened persistent storage bucket" << id;
        if (error) {
            error->clear();
        }
        return storage;
    }

    bool StorageModuleExtension::commitPersistentStorage(const QString &id, const StorageItems &items, StorageError *error) {
        auto proposedStorages = m_persistentItems;
        proposedStorages.insert(id, items);
        if (!writePersistentStorage(proposedStorages, error)) {
            return false;
        }
        m_persistentItems = std::move(proposedStorages);
        return true;
    }

    void StorageModuleExtension::loadPersistentStorage() {
        QFile file(m_persistentFilePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return;
        }
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            return;
        }
        const auto root = document.object();
        for (auto storageIt = root.begin(); storageIt != root.end(); ++storageIt) {
            if (!storageIt.value().isObject()) {
                continue;
            }
            StorageItems items;
            const auto bucket = storageIt.value().toObject();
            for (auto itemIt = bucket.begin(); itemIt != bucket.end(); ++itemIt) {
                if (itemIt.value().isString()) {
                    items.insert(itemIt.key(), itemIt.value().toString());
                }
            }
            m_persistentItems.insert(storageIt.key(), items);
        }
        qCDebug(lcStorageModule) << "Loaded persistent storage buckets" << m_persistentItems.size();
    }

    bool StorageModuleExtension::writePersistentStorage(const QMap<QString, StorageItems> &storages, StorageError *error) const {
        const QFileInfo fileInfo(m_persistentFilePath);
        if (!QDir().mkpath(fileInfo.absolutePath())) {
            StorageInterfacePrivate::setError(error, StorageErrorCode::Io, {}, {}, StorageInterface::tr("Failed to create the script storage directory."));
            qCCritical(lcStorageModule) << "Failed to create script storage directory" << fileInfo.absolutePath();
            return false;
        }

        QJsonObject root;
        for (auto storageIt = storages.cbegin(); storageIt != storages.cend(); ++storageIt) {
            QJsonObject bucket;
            for (auto itemIt = storageIt.value().cbegin(); itemIt != storageIt.value().cend(); ++itemIt) {
                bucket.insert(itemIt.key(), itemIt.value());
            }
            root.insert(storageIt.key(), bucket);
        }
        const auto data = QJsonDocument(root).toJson(QJsonDocument::Indented);

        QSaveFile file(m_persistentFilePath);
        file.setDirectWriteFallback(false);
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
            StorageInterfacePrivate::setError(error, StorageErrorCode::Io, {}, {}, StorageInterface::tr("Failed to save persistent script storage: %1").arg(file.errorString()));
            qCCritical(lcStorageModule) << "Failed to save persistent script storage" << m_persistentFilePath << file.errorString();
            return false;
        }
        if (error) {
            error->clear();
        }
        qCDebug(lcStorageModule) << "Committed persistent storage buckets" << storages.size();
        return true;
    }

}

#include "StorageModuleExtension.moc"
#include "moc_StorageModuleExtension.cpp"
