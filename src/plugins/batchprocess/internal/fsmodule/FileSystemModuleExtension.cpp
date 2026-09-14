// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "FileSystemModuleExtension.h"

#include <cmath>
#include <optional>

#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QLoggingCategory>
#include <QPointer>
#include <QStringList>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/FileSystemAccessInterface.h>
#include <batchprocess/FileSystemTypes.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/private/FileSystemAccessInterface_p.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcFileSystemModule, "diffscope.batchprocess.fsmodule")

    namespace {

        constexpr auto ModuleName = "diffscope:fs";
        constexpr auto ModuleResourcePath = ":/diffscope/batchprocess/internalscripts/fs/fs-module.js";

        QString javaScriptErrorText(const QJSValue &error) {
            auto text = error.toString();
            const auto stack = error.property(QStringLiteral("stack")).toString();
            if (!stack.isEmpty() && !text.contains(stack)) {
                text.append(QLatin1Char('\n'));
                text.append(stack);
            }
            return text;
        }

        QString accessName(FileAccessMode access) {
            switch (access) {
                case FileAccessMode::Read:
                    return QStringLiteral("read");
                case FileAccessMode::Write:
                    return QStringLiteral("write");
                case FileAccessMode::ReadWrite:
                    return QStringLiteral("readWrite");
            }
            return QStringLiteral("read");
        }

        std::optional<FileAccessMode> accessMode(int value) {
            switch (value) {
                case 0:
                    return FileAccessMode::Read;
                case 1:
                    return FileAccessMode::Write;
                case 2:
                    return FileAccessMode::ReadWrite;
                default:
                    return std::nullopt;
            }
        }

        std::optional<FileWriteDisposition> writeDisposition(int value) {
            switch (value) {
                case 0:
                    return FileWriteDisposition::Replace;
                case 1:
                    return FileWriteDisposition::CreateNew;
                case 2:
                    return FileWriteDisposition::Append;
                default:
                    return std::nullopt;
            }
        }

        QString entryKindName(FileSystemEntryKind kind) {
            switch (kind) {
                case FileSystemEntryKind::File:
                    return QStringLiteral("file");
                case FileSystemEntryKind::Directory:
                    return QStringLiteral("directory");
                case FileSystemEntryKind::SymbolicLink:
                    return QStringLiteral("symbolicLink");
                case FileSystemEntryKind::Other:
                    return QStringLiteral("other");
            }
            return QStringLiteral("other");
        }

        QString errorCodeName(FileSystemErrorCode code) {
            switch (code) {
                case FileSystemErrorCode::None:
                    return QStringLiteral("unknown");
                case FileSystemErrorCode::NotFound:
                    return QStringLiteral("notFound");
                case FileSystemErrorCode::AlreadyExists:
                    return QStringLiteral("alreadyExists");
                case FileSystemErrorCode::NotFile:
                    return QStringLiteral("notFile");
                case FileSystemErrorCode::NotDirectory:
                    return QStringLiteral("notDirectory");
                case FileSystemErrorCode::NotEmpty:
                    return QStringLiteral("notEmpty");
                case FileSystemErrorCode::InvalidPath:
                    return QStringLiteral("invalidPath");
                case FileSystemErrorCode::InvalidState:
                    return QStringLiteral("invalidState");
                case FileSystemErrorCode::ReadOnly:
                    return QStringLiteral("readOnly");
                case FileSystemErrorCode::Busy:
                    return QStringLiteral("busy");
                case FileSystemErrorCode::OutOfSpace:
                    return QStringLiteral("outOfSpace");
                case FileSystemErrorCode::CrossDevice:
                    return QStringLiteral("crossDevice");
                case FileSystemErrorCode::NotSupported:
                    return QStringLiteral("notSupported");
                case FileSystemErrorCode::TooLarge:
                    return QStringLiteral("tooLarge");
                case FileSystemErrorCode::Io:
                    return QStringLiteral("io");
                case FileSystemErrorCode::Encoding:
                    return QStringLiteral("encoding");
                case FileSystemErrorCode::Unknown:
                    return QStringLiteral("unknown");
            }
            return QStringLiteral("unknown");
        }

        std::optional<qint64> maximumBytes(double value) {
            if (value < 0) {
                return std::nullopt;
            }
            return static_cast<qint64>(value);
        }

        bool isValidMaximumBytes(double value) {
            return value == -1 || (std::isfinite(value) && value >= 0 && std::floor(value) == value && value <= 9007199254740991.0);
        }

        std::optional<bool> atomicOption(int value) {
            if (value < 0) {
                return std::nullopt;
            }
            return value != 0;
        }

        QStringList stringList(const QJSValue &array) {
            QStringList result;
            const auto length = array.property(QStringLiteral("length")).toUInt();
            result.reserve(static_cast<qsizetype>(length));
            for (quint32 index = 0; index < length; ++index) {
                result.append(array.property(index).toString());
            }
            return result;
        }

    }

    class FileSystemModuleBridge;

    class FileSystemHandleBridge : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString kind READ kind CONSTANT)
        Q_PROPERTY(QString path READ path CONSTANT)
        Q_PROPERTY(QString access READ access CONSTANT)
        Q_PROPERTY(bool recursive READ recursive CONSTANT)
    public:
        FileSystemHandleBridge(FileSystemModuleBridge *moduleBridge, const FileSystemHandle &handle, QObject *parent);

        QString kind() const;
        QString path() const;
        QString access() const;
        bool recursive() const;

        Q_INVOKABLE QJSValue stat(bool followSymbolicLinks) const;
        Q_INVOKABLE QJSValue readText(const QString &encoding, double maximumBytes) const;
        Q_INVOKABLE QJSValue readBytes(double maximumBytes) const;
        Q_INVOKABLE QJSValue writeText(const QString &text, const QString &encoding, int disposition, int atomic, bool createParents) const;
        Q_INVOKABLE QJSValue writeBytes(const QByteArray &data, int disposition, int atomic, bool createParents) const;
        Q_INVOKABLE QJSValue entries(bool recursive, bool files, bool directories, const QJSValue &nameFilters) const;
        Q_INVOKABLE QJSValue file(const QString &relativePath, int access) const;
        Q_INVOKABLE QJSValue directory(const QString &relativePath) const;

        FileSystemHandle handle() const;

    private:
        QPointer<FileSystemModuleBridge> m_moduleBridge;
        FileSystemHandle m_handle;
    };

    class FileSystemModuleBridge : public QObject {
        Q_OBJECT
    public:
        FileSystemModuleBridge(FileSystemAccessInterface *fileSystemAccessInterface, ScriptExecutionContext *context, QJSEngine *engine)
            : QObject(engine), m_fileSystemAccessInterface(fileSystemAccessInterface), m_context(context), m_engine(engine) {
        }

        ScriptExecutionContext *context() const {
            return m_context.data();
        }

        QJSValue success() const {
            auto result = m_engine->newObject();
            result.setProperty(QStringLiteral("ok"), true);
            result.setProperty(QStringLiteral("value"), QJSValue(QJSValue::UndefinedValue));
            return result;
        }

        QJSValue success(const QJSValue &value) const {
            auto result = m_engine->newObject();
            result.setProperty(QStringLiteral("ok"), true);
            result.setProperty(QStringLiteral("value"), value);
            return result;
        }

        QJSValue failure(const FileSystemError &error) const {
            qCWarning(lcFileSystemModule) << "File-system operation failed" << error.operation << errorCodeName(error.code) << error.path << error.destinationPath << error.message;
            auto errorObject = m_engine->newObject();
            errorObject.setProperty(QStringLiteral("permissionDenied"), error.kind == FileSystemErrorKind::PermissionDenied);
            errorObject.setProperty(QStringLiteral("code"), errorCodeName(error.code));
            errorObject.setProperty(QStringLiteral("message"), error.message);
            errorObject.setProperty(QStringLiteral("operation"), error.operation);
            errorObject.setProperty(QStringLiteral("path"), error.path.isEmpty() ? QJSValue(QJSValue::NullValue) : QJSValue(error.path));
            errorObject.setProperty(QStringLiteral("destinationPath"), error.destinationPath.isEmpty() ? QJSValue(QJSValue::NullValue) : QJSValue(error.destinationPath));
            auto result = m_engine->newObject();
            result.setProperty(QStringLiteral("ok"), false);
            result.setProperty(QStringLiteral("error"), errorObject);
            return result;
        }

        QJSValue invalidArgument(const QString &operation, const QString &message) const {
            FileSystemError error;
            error.kind = FileSystemErrorKind::FileSystem;
            error.code = FileSystemErrorCode::InvalidPath;
            error.operation = operation;
            error.message = message;
            return failure(error);
        }

        QJSValue handleResult(const FileSystemHandle &handle, const FileSystemError &error) const {
            if (!handle.isValid()) {
                return failure(error);
            }
            return success(m_engine->newQObject(new FileSystemHandleBridge(const_cast<FileSystemModuleBridge *>(this), handle, m_engine)));
        }

        QJSValue statResult(const FileSystemStat &stat) const {
            auto value = m_engine->newObject();
            value.setProperty(QStringLiteral("path"), stat.path);
            value.setProperty(QStringLiteral("canonicalPath"), stat.canonicalPath.isEmpty() ? QJSValue(QJSValue::NullValue) : QJSValue(stat.canonicalPath));
            value.setProperty(QStringLiteral("exists"), stat.exists);
            value.setProperty(QStringLiteral("kind"), stat.kind ? QJSValue(entryKindName(*stat.kind)) : QJSValue(QJSValue::NullValue));
            value.setProperty(QStringLiteral("resolvedKind"), stat.resolvedKind ? QJSValue(entryKindName(*stat.resolvedKind)) : QJSValue(QJSValue::NullValue));
            value.setProperty(QStringLiteral("size"), stat.size ? QJSValue(static_cast<double>(*stat.size)) : QJSValue(QJSValue::NullValue));
            value.setProperty(QStringLiteral("createdAt"), stat.createdAt.isValid() ? m_engine->toScriptValue(stat.createdAt) : QJSValue(QJSValue::NullValue));
            value.setProperty(QStringLiteral("modifiedAt"), stat.modifiedAt.isValid() ? m_engine->toScriptValue(stat.modifiedAt) : QJSValue(QJSValue::NullValue));
            value.setProperty(QStringLiteral("accessedAt"), stat.accessedAt.isValid() ? m_engine->toScriptValue(stat.accessedAt) : QJSValue(QJSValue::NullValue));
            value.setProperty(QStringLiteral("metadataChangedAt"), stat.metadataChangedAt.isValid() ? m_engine->toScriptValue(stat.metadataChangedAt) : QJSValue(QJSValue::NullValue));
            value.setProperty(QStringLiteral("symbolicLinkTarget"), stat.symbolicLinkTarget.isEmpty() ? QJSValue(QJSValue::NullValue) : QJSValue(stat.symbolicLinkTarget));
            value.setProperty(QStringLiteral("hidden"), stat.hidden);
            value.setProperty(QStringLiteral("readable"), stat.readable ? QJSValue(*stat.readable) : QJSValue(QJSValue::NullValue));
            value.setProperty(QStringLiteral("writable"), stat.writable ? QJSValue(*stat.writable) : QJSValue(QJSValue::NullValue));
            value.setProperty(QStringLiteral("executable"), stat.executable ? QJSValue(*stat.executable) : QJSValue(QJSValue::NullValue));
            return success(value);
        }

        Q_INVOKABLE QJSValue requestPermission(const QString &pathPattern, int access, const QString &reason) const {
            const auto accessValue = accessMode(access);
            if (!accessValue) {
                return invalidArgument(QStringLiteral("permission"), FileSystemAccessInterface::tr("The requested access mode is invalid."));
            }
            FileSystemError error;
            const auto decision = m_fileSystemAccessInterface->requestPermission(context(), {pathPattern, *accessValue, reason}, &error);
            if (decision == FileSystemPermissionDecision::Failed) {
                return failure(error);
            }
            return success(QJSValue(decision == FileSystemPermissionDecision::Allowed));
        }

        Q_INVOKABLE QJSValue stat(const QString &path, bool followSymbolicLinks) const {
            FileSystemStat stat;
            FileSystemError error;
            if (!m_fileSystemAccessInterface->stat(context(), path, {followSymbolicLinks}, &stat, &error)) {
                return failure(error);
            }
            return statResult(stat);
        }

        Q_INVOKABLE QJSValue getFile(const QString &path, int access) const {
            const auto accessValue = accessMode(access);
            if (!accessValue) {
                return invalidArgument(QStringLiteral("getFile"), FileSystemAccessInterface::tr("The requested access mode is invalid."));
            }
            FileSystemError error;
            return handleResult(m_fileSystemAccessInterface->getFile(context(), path, *accessValue, &error), error);
        }

        Q_INVOKABLE QJSValue getDirectory(const QString &path, int access, bool recursive) const {
            const auto accessValue = accessMode(access);
            if (!accessValue) {
                return invalidArgument(QStringLiteral("getDirectory"), FileSystemAccessInterface::tr("The requested access mode is invalid."));
            }
            FileSystemError error;
            return handleResult(m_fileSystemAccessInterface->getDirectory(context(), path, *accessValue, recursive, &error), error);
        }

        Q_INVOKABLE QJSValue scriptPackage() const {
            FileSystemError error;
            return handleResult(m_fileSystemAccessInterface->scriptPackage(context(), &error), error);
        }

        Q_INVOKABLE QJSValue dataDirectory() const {
            FileSystemError error;
            return handleResult(m_fileSystemAccessInterface->dataDirectory(context(), &error), error);
        }

        Q_INVOKABLE QJSValue temporaryDirectory() const {
            FileSystemError error;
            return handleResult(m_fileSystemAccessInterface->temporaryDirectory(context(), &error), error);
        }

        Q_INVOKABLE QJSValue createDirectory(const QString &path, bool recursive) const {
            FileSystemError error;
            if (!m_fileSystemAccessInterface->createDirectory(context(), path, {recursive}, &error)) {
                return failure(error);
            }
            return success();
        }

        Q_INVOKABLE QJSValue copy(const QString &source, const QString &destination, bool overwrite, bool recursive) const {
            FileSystemError error;
            if (!m_fileSystemAccessInterface->copy(context(), source, destination, {overwrite, recursive}, &error)) {
                return failure(error);
            }
            return success();
        }

        Q_INVOKABLE QJSValue move(const QString &source, const QString &destination, bool overwrite) const {
            FileSystemError error;
            if (!m_fileSystemAccessInterface->move(context(), source, destination, {overwrite}, &error)) {
                return failure(error);
            }
            return success();
        }

        Q_INVOKABLE QJSValue remove(const QString &path, bool recursive, bool useTrash) const {
            FileSystemError error;
            if (!m_fileSystemAccessInterface->remove(context(), path, {recursive, useTrash}, &error)) {
                return failure(error);
            }
            return success();
        }

        Q_INVOKABLE QJSValue realPath(const QString &path) const {
            QString value;
            FileSystemError error;
            if (!m_fileSystemAccessInterface->realPath(context(), path, &value, &error)) {
                return failure(error);
            }
            return success(QJSValue(value));
        }

        Q_INVOKABLE QString normalize(const QString &path) const {
            return m_fileSystemAccessInterface->normalize(path);
        }

        Q_INVOKABLE QJSValue resolve(const QJSValue &parts) const {
            QString value;
            FileSystemError error;
            if (!m_fileSystemAccessInterface->resolve(context(), stringList(parts), &value, &error)) {
                return failure(error);
            }
            return success(QJSValue(value));
        }

        Q_INVOKABLE QJSValue relative(const QString &from, const QString &to) const {
            QString value;
            FileSystemError error;
            if (!m_fileSystemAccessInterface->relative(context(), from, to, &value, &error)) {
                return failure(error);
            }
            return success(QJSValue(value));
        }

        Q_INVOKABLE bool isAbsolute(const QString &path) const {
            return m_fileSystemAccessInterface->isAbsolute(path);
        }

        Q_INVOKABLE QString join(const QJSValue &parts) const {
            return m_fileSystemAccessInterface->join(stringList(parts));
        }

        Q_INVOKABLE QString baseName(const QString &path) const {
            return m_fileSystemAccessInterface->baseName(path);
        }

        Q_INVOKABLE QString directoryName(const QString &path) const {
            return m_fileSystemAccessInterface->directoryName(path);
        }

        Q_INVOKABLE QString extension(const QString &path) const {
            return m_fileSystemAccessInterface->extension(path);
        }

        Q_INVOKABLE QJSValue parse(const QString &path) const {
            const auto parsed = m_fileSystemAccessInterface->parse(path);
            auto value = m_engine->newObject();
            value.setProperty(QStringLiteral("root"), parsed.root);
            value.setProperty(QStringLiteral("directory"), parsed.directory);
            value.setProperty(QStringLiteral("baseName"), parsed.baseName);
            value.setProperty(QStringLiteral("name"), parsed.name);
            value.setProperty(QStringLiteral("extension"), parsed.extension);
            return value;
        }

        Q_INVOKABLE QJSValue format(const QString &root, const QString &directory, const QString &baseName, const QString &name, const QString &extension) const {
            QString value;
            FileSystemError error;
            if (!m_fileSystemAccessInterface->format({root, directory, baseName, name, extension}, &value, &error)) {
                return failure(error);
            }
            return success(QJSValue(value));
        }

        Q_INVOKABLE QJSValue matchesPattern(const QString &path, const QString &pattern) const {
            bool value = false;
            FileSystemError error;
            if (!m_fileSystemAccessInterface->matchesPattern(context(), path, pattern, &value, &error)) {
                return failure(error);
            }
            return success(QJSValue(value));
        }

        FileSystemAccessInterface *fileSystemAccessInterface() const {
            return m_fileSystemAccessInterface;
        }

        QJSEngine *engine() const {
            return m_engine;
        }

    private:
        FileSystemAccessInterface *m_fileSystemAccessInterface;
        QPointer<ScriptExecutionContext> m_context;
        QJSEngine *m_engine;
    };

    FileSystemHandleBridge::FileSystemHandleBridge(FileSystemModuleBridge *moduleBridge, const FileSystemHandle &handle, QObject *parent)
        : QObject(parent), m_moduleBridge(moduleBridge), m_handle(handle) {
    }

    QString FileSystemHandleBridge::kind() const {
        return m_handle.kind() == FileSystemHandle::File ? QStringLiteral("file") : QStringLiteral("directory");
    }

    QString FileSystemHandleBridge::path() const {
        return m_handle.path();
    }

    QString FileSystemHandleBridge::access() const {
        return accessName(m_handle.access());
    }

    bool FileSystemHandleBridge::recursive() const {
        return m_handle.recursive();
    }

    QJSValue FileSystemHandleBridge::stat(bool followSymbolicLinks) const {
        if (!m_moduleBridge) {
            return {};
        }
        FileSystemStat stat;
        FileSystemError error;
        if (!m_moduleBridge->fileSystemAccessInterface()->stat(m_moduleBridge->context(), m_handle, {followSymbolicLinks}, &stat, &error)) {
            return m_moduleBridge->failure(error);
        }
        return m_moduleBridge->statResult(stat);
    }

    QJSValue FileSystemHandleBridge::readText(const QString &encoding, double maximumByteCount) const {
        if (!m_moduleBridge) {
            return {};
        }
        if (!isValidMaximumBytes(maximumByteCount)) {
            return m_moduleBridge->invalidArgument(QStringLiteral("read"), FileSystemAccessInterface::tr("The maximum byte count is invalid."));
        }
        QString value;
        FileSystemError error;
        if (!m_moduleBridge->fileSystemAccessInterface()->readText(m_moduleBridge->context(), m_handle, {encoding, maximumBytes(maximumByteCount)}, &value, &error)) {
            return m_moduleBridge->failure(error);
        }
        return m_moduleBridge->success(QJSValue(value));
    }

    QJSValue FileSystemHandleBridge::readBytes(double maximumByteCount) const {
        if (!m_moduleBridge) {
            return {};
        }
        if (!isValidMaximumBytes(maximumByteCount)) {
            return m_moduleBridge->invalidArgument(QStringLiteral("read"), FileSystemAccessInterface::tr("The maximum byte count is invalid."));
        }
        QByteArray value;
        FileSystemError error;
        if (!m_moduleBridge->fileSystemAccessInterface()->readBytes(m_moduleBridge->context(), m_handle, {maximumBytes(maximumByteCount)}, &value, &error)) {
            return m_moduleBridge->failure(error);
        }
        return m_moduleBridge->success(m_moduleBridge->engine()->toScriptValue(value));
    }

    QJSValue FileSystemHandleBridge::writeText(const QString &text, const QString &encoding, int disposition, int atomic, bool createParents) const {
        if (!m_moduleBridge) {
            return {};
        }
        const auto dispositionValue = writeDisposition(disposition);
        if (!dispositionValue || atomic < -1 || atomic > 1) {
            return m_moduleBridge->invalidArgument(QStringLiteral("write"), FileSystemAccessInterface::tr("The write disposition is invalid."));
        }
        FileSystemError error;
        if (!m_moduleBridge->fileSystemAccessInterface()->writeText(m_moduleBridge->context(), m_handle, text, {encoding, *dispositionValue, atomicOption(atomic), createParents}, &error)) {
            return m_moduleBridge->failure(error);
        }
        return m_moduleBridge->success();
    }

    QJSValue FileSystemHandleBridge::writeBytes(const QByteArray &data, int disposition, int atomic, bool createParents) const {
        if (!m_moduleBridge) {
            return {};
        }
        const auto dispositionValue = writeDisposition(disposition);
        if (!dispositionValue || atomic < -1 || atomic > 1) {
            return m_moduleBridge->invalidArgument(QStringLiteral("write"), FileSystemAccessInterface::tr("The write disposition is invalid."));
        }
        FileSystemError error;
        if (!m_moduleBridge->fileSystemAccessInterface()->writeBytes(m_moduleBridge->context(), m_handle, data, {*dispositionValue, atomicOption(atomic), createParents}, &error)) {
            return m_moduleBridge->failure(error);
        }
        return m_moduleBridge->success();
    }

    QJSValue FileSystemHandleBridge::entries(bool recursive, bool files, bool directories, const QJSValue &nameFilters) const {
        if (!m_moduleBridge) {
            return {};
        }
        QList<FileSystemDirectoryEntry> entries;
        FileSystemError error;
        if (!m_moduleBridge->fileSystemAccessInterface()->entries(m_moduleBridge->context(), m_handle, {recursive, files, directories, stringList(nameFilters)}, &entries, &error)) {
            return m_moduleBridge->failure(error);
        }
        auto array = m_moduleBridge->engine()->newArray(static_cast<quint32>(entries.size()));
        for (qsizetype index = 0; index < entries.size(); ++index) {
            const auto &entry = entries.at(index);
            auto value = m_moduleBridge->engine()->newObject();
            value.setProperty(QStringLiteral("relativePath"), entry.relativePath);
            value.setProperty(QStringLiteral("path"), entry.path);
            value.setProperty(QStringLiteral("kind"), entryKindName(entry.kind));
            value.setProperty(QStringLiteral("resolvedKind"), entry.resolvedKind ? QJSValue(entryKindName(*entry.resolvedKind)) : QJSValue(QJSValue::NullValue));
            array.setProperty(static_cast<quint32>(index), value);
        }
        return m_moduleBridge->success(array);
    }

    QJSValue FileSystemHandleBridge::file(const QString &relativePath, int access) const {
        if (!m_moduleBridge) {
            return {};
        }
        const auto accessValue = accessMode(access);
        if (!accessValue) {
            return m_moduleBridge->invalidArgument(QStringLiteral("getFile"), FileSystemAccessInterface::tr("The requested access mode is invalid."));
        }
        FileSystemError error;
        return m_moduleBridge->handleResult(m_moduleBridge->fileSystemAccessInterface()->childFile(m_moduleBridge->context(), m_handle, relativePath, *accessValue, &error), error);
    }

    QJSValue FileSystemHandleBridge::directory(const QString &relativePath) const {
        if (!m_moduleBridge) {
            return {};
        }
        FileSystemError error;
        return m_moduleBridge->handleResult(m_moduleBridge->fileSystemAccessInterface()->childDirectory(m_moduleBridge->context(), m_handle, relativePath, &error), error);
    }

    FileSystemHandle FileSystemHandleBridge::handle() const {
        return m_handle;
    }

    FileSystemModuleExtension::FileSystemModuleExtension(BatchProcessInterface *batchProcessInterface, FileSystemAccessInterface *fileSystemAccessInterface, QObject *parent)
        : QObject(parent), m_batchProcessInterface(batchProcessInterface), m_fileSystemAccessInterface(fileSystemAccessInterface) {
        Q_ASSERT(m_batchProcessInterface);
        Q_ASSERT(m_fileSystemAccessInterface);
        m_fileSystemAccessInterface->d_func()->moduleExtension = this;

        const auto extensionRegistered = m_batchProcessInterface->registerEngineExtension(this, [this](QJSEngine *engine, ScriptExecutionContext *context) {
            installIntoEngine(engine, context);
        });
        const auto moduleRegistered = m_batchProcessInterface->registerModule(QString::fromLatin1(ModuleName), this, [this](ScriptExecutionContext *context) {
            return createModule(context);
        });
        const auto startRegistered = m_batchProcessInterface->registerActionStartedCallback(this, [this](ScriptExecutionContext *context) {
            m_fileSystemAccessInterface->d_func()->beginExecution(context);
        });
        const auto finishRegistered = m_batchProcessInterface->registerActionFinishedCallback(this, [this](ScriptExecutionContext *context) {
            m_fileSystemAccessInterface->d_func()->scheduleEndExecution(context);
        });
        if (!extensionRegistered || !moduleRegistered || !startRegistered || !finishRegistered) {
            qFatal("Failed to register the Batch Process file-system module");
        }
        qCDebug(lcFileSystemModule) << "Registered file-system module" << ModuleName;
    }

    FileSystemModuleExtension::~FileSystemModuleExtension() = default;

    void FileSystemModuleExtension::installIntoEngine(QJSEngine *engine, ScriptExecutionContext *context) {
        if (!engine || !context || context->engine() != engine) {
            qFatal("The Batch Process file-system engine extension received an invalid execution context");
        }
        QFile moduleFile(QString::fromLatin1(ModuleResourcePath));
        if (!moduleFile.open(QIODevice::ReadOnly)) {
            qFatal() << "Failed to open the embedded Batch Process file-system module:" << moduleFile.errorString();
        }
        const auto factory = engine->evaluate(QString::fromUtf8(moduleFile.readAll()), QString::fromLatin1(ModuleResourcePath));
        if (factory.isError()) {
            qFatal() << "Failed to evaluate the embedded Batch Process file-system module:" << javaScriptErrorText(factory);
        }
        if (!factory.isCallable()) {
            qFatal("The embedded Batch Process file-system module did not return a factory function");
        }
        auto bridge = new FileSystemModuleBridge(m_fileSystemAccessInterface, context, engine);
        const auto bundle = factory.call({engine->newQObject(bridge)});
        if (engine->hasError()) {
            qFatal() << "Failed to create the Batch Process file-system module:" << javaScriptErrorText(engine->catchError());
        }
        if (bundle.isError()) {
            qFatal() << "Failed to create the Batch Process file-system module:" << javaScriptErrorText(bundle);
        }
        if (!bundle.isObject() || !bundle.property(QStringLiteral("exports")).isObject() || !bundle.property(QStringLiteral("wrapHandle")).isCallable() || !bundle.property(QStringLiteral("unwrapHandle")).isCallable()) {
            qFatal("The embedded Batch Process file-system module factory returned an invalid helper bundle");
        }
        const auto exports = bundle.property(QStringLiteral("exports"));
        m_engineBundles.insert(engine, bundle);
        connect(context, &QObject::destroyed, this, [this, engine] {
            m_engineBundles.remove(engine);
        });
    }

    QJSValue FileSystemModuleExtension::createModule(ScriptExecutionContext *context) const {
        FileSystemError error;
        const auto exports = helper(context, QStringLiteral("exports"), &error);
        if (!exports.isObject()) {
            qFatal() << "The embedded Batch Process file-system module exports are unavailable:" << error.message;
        }
        return exports;
    }

    QJSValue FileSystemModuleExtension::helper(ScriptExecutionContext *context, const QString &name, FileSystemError *error) const {
        if (!context || !context->engine()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), {}, FileSystemAccessInterface::tr("The JavaScript execution context is not available."));
            return {};
        }
        const auto bundleIt = m_engineBundles.constFind(context->engine());
        if (bundleIt == m_engineBundles.cend()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), {}, FileSystemAccessInterface::tr("The JavaScript file-system module is not installed in this engine."));
            return {};
        }
        const auto bundle = bundleIt.value();
        const auto value = bundle.property(name);
        if (!bundle.isObject() || !bundle.hasProperty(name)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), {}, FileSystemAccessInterface::tr("The JavaScript file-system module is not installed in this engine."));
            return {};
        }
        return value;
    }

    QJSValue FileSystemModuleExtension::wrapHandle(ScriptExecutionContext *context, const FileSystemHandle &handle, FileSystemError *error) const {
        const auto wrap = helper(context, QStringLiteral("wrapHandle"), error);
        if (!wrap.isCallable()) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        auto engine = context->engine();
        const auto moduleBridge = qobject_cast<FileSystemModuleBridge *>(helper(context, QStringLiteral("bridge"), error).toQObject());
        if (!moduleBridge) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), handle.path(), FileSystemAccessInterface::tr("The JavaScript file-system bridge is not available."));
            return QJSValue(QJSValue::UndefinedValue);
        }
        auto bridge = new FileSystemHandleBridge(moduleBridge, handle, engine);
        const auto value = wrap.call({engine->newQObject(bridge)});
        if (value.isError() || engine->hasError()) {
            const auto jsError = engine->hasError() ? engine->catchError() : value;
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), handle.path(), javaScriptErrorText(jsError));
            return QJSValue(QJSValue::UndefinedValue);
        }
        return value;
    }

    bool FileSystemModuleExtension::unwrapHandle(ScriptExecutionContext *context, const QJSValue &value, FileSystemHandle *handle, FileSystemHandle::Kind expectedKind, FileSystemError *error) const {
        const auto unwrap = helper(context, QStringLiteral("unwrapHandle"), error);
        if (!unwrap.isCallable()) {
            return false;
        }
        auto engine = context->engine();
        const auto unwrapped = unwrap.call({value});
        if (unwrapped.isError() || engine->hasError()) {
            if (engine->hasError()) {
                engine->catchError();
            }
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), {}, FileSystemAccessInterface::tr("The value is not a file-system handle from the current JavaScript engine."));
            return false;
        }
        const auto bridge = qobject_cast<FileSystemHandleBridge *>(unwrapped.toQObject());
        if (!bridge) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), {}, FileSystemAccessInterface::tr("The value is not a file-system handle from the current JavaScript engine."));
            return false;
        }
        const auto candidate = bridge->handle();
        if (!m_fileSystemAccessInterface->validateHandle(context, candidate, candidate.access(), expectedKind, error)) {
            return false;
        }
        *handle = candidate;
        return true;
    }

}

#include "FileSystemModuleExtension.moc"
#include "moc_FileSystemModuleExtension.cpp"
