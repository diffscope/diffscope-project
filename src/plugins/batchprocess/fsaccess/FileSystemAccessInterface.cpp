// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "FileSystemAccessInterface.h"
#include "FileSystemAccessInterface_p.h"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <system_error>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QSaveFile>
#include <QStringConverter>

#include <CoreApi/applicationinfo.h>

#include <coreplugin/ActionWindowInterfaceBase.h>

#include <batchprocess/Script.h>
#include <batchprocess/ScriptAction.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/internal/BatchProcessSettings.h>
#include <batchprocess/internal/FileSystemModuleExtension.h>
#include <batchprocess/internal/FileSystemPathUtils.h>
#include <batchprocess/internal/FileSystemPermissionDialog.h>
#include <batchprocess/private/Script_p.h>

namespace BatchProcess {

    Q_STATIC_LOGGING_CATEGORY(lcFileSystemAccess, "diffscope.batchprocess.fsaccess")

    static FileSystemAccessInterface *m_instance = nullptr;

    namespace {

        int accessMask(FileAccessMode access) {
            switch (access) {
                case FileAccessMode::Read:
                    return 1;
                case FileAccessMode::Write:
                    return 2;
                case FileAccessMode::ReadWrite:
                    return 3;
            }
            return 0;
        }

        QString nativePath(const QString &path) {
            return QDir::toNativeSeparators(Internal::normalizedFileSystemPath(path));
        }

        FileSystemEntryKind entryKind(const QFileInfo &fileInfo) {
            if (fileInfo.isSymbolicLink() || fileInfo.isJunction()) {
                return FileSystemEntryKind::SymbolicLink;
            }
            if (fileInfo.isFile()) {
                return FileSystemEntryKind::File;
            }
            if (fileInfo.isDir()) {
                return FileSystemEntryKind::Directory;
            }
            return FileSystemEntryKind::Other;
        }

        std::optional<FileSystemEntryKind> resolvedEntryKind(const QFileInfo &fileInfo) {
            if (!fileInfo.exists()) {
                return std::nullopt;
            }
            if (fileInfo.isFile()) {
                return FileSystemEntryKind::File;
            }
            if (fileInfo.isDir()) {
                return FileSystemEntryKind::Directory;
            }
            return FileSystemEntryKind::Other;
        }

        QString canonicalOrDerivedPath(const QString &path) {
            QFileInfo fileInfo(path);
            const auto canonicalPath = fileInfo.canonicalFilePath();
            if (!canonicalPath.isEmpty()) {
                return Internal::normalizedFileSystemPath(canonicalPath);
            }

            auto unresolvedSuffix = QStringList{};
            auto currentPath = Internal::normalizedFileSystemPath(path);
            while (!currentPath.isEmpty()) {
                QFileInfo currentInfo(currentPath);
                const auto existingCanonicalPath = currentInfo.canonicalFilePath();
                if (!existingCanonicalPath.isEmpty()) {
                    auto result = Internal::normalizedFileSystemPath(existingCanonicalPath);
                    for (auto it = unresolvedSuffix.crbegin(); it != unresolvedSuffix.crend(); ++it) {
                        result = Internal::normalizedFileSystemPath(QDir(result).filePath(*it));
                    }
                    return result;
                }
                const auto name = currentInfo.fileName();
                const auto parentPath = Internal::normalizedFileSystemPath(currentInfo.path());
                if (name.isEmpty() || parentPath == currentPath) {
                    break;
                }
                unresolvedSuffix.append(name);
                currentPath = parentPath;
            }
            return {};
        }

        FileSystemErrorCode errorCodeForFileError(QFileDevice::FileError error) {
            switch (error) {
                case QFileDevice::PermissionsError:
                    return FileSystemErrorCode::ReadOnly;
                case QFileDevice::ResourceError:
                    return FileSystemErrorCode::OutOfSpace;
                case QFileDevice::OpenError:
                case QFileDevice::ReadError:
                case QFileDevice::WriteError:
                case QFileDevice::RemoveError:
                case QFileDevice::RenameError:
                case QFileDevice::CopyError:
                    return FileSystemErrorCode::Io;
                case QFileDevice::NoError:
                    return FileSystemErrorCode::None;
                default:
                    return FileSystemErrorCode::Unknown;
            }
        }

        FileSystemErrorCode errorCodeForSystemError(const std::error_code &error) {
            if (!error) {
                return FileSystemErrorCode::None;
            }
            if (error == std::errc::no_such_file_or_directory) {
                return FileSystemErrorCode::NotFound;
            }
            if (error == std::errc::file_exists) {
                return FileSystemErrorCode::AlreadyExists;
            }
            if (error == std::errc::directory_not_empty) {
                return FileSystemErrorCode::NotEmpty;
            }
            if (error == std::errc::not_a_directory) {
                return FileSystemErrorCode::NotDirectory;
            }
            if (error == std::errc::is_a_directory) {
                return FileSystemErrorCode::NotFile;
            }
            if (error == std::errc::permission_denied || error == std::errc::read_only_file_system) {
                return FileSystemErrorCode::ReadOnly;
            }
            if (error == std::errc::device_or_resource_busy) {
                return FileSystemErrorCode::Busy;
            }
            if (error == std::errc::no_space_on_device) {
                return FileSystemErrorCode::OutOfSpace;
            }
            if (error == std::errc::cross_device_link) {
                return FileSystemErrorCode::CrossDevice;
            }
            if (error == std::errc::operation_not_supported || error == std::errc::function_not_supported) {
                return FileSystemErrorCode::NotSupported;
            }
            if (error == std::errc::invalid_argument || error == std::errc::filename_too_long) {
                return FileSystemErrorCode::InvalidPath;
            }
            if (error == std::errc::file_too_large) {
                return FileSystemErrorCode::TooLarge;
            }
            return FileSystemErrorCode::Io;
        }

        std::filesystem::path nativeFileSystemPath(const QString &path) {
#if defined(Q_OS_WIN)
            return std::filesystem::path(path.toStdWString());
#else
            return std::filesystem::path(path.toUtf8().constData());
#endif
        }

        bool pathIsInvalidated(const FileSystemAccessInterfacePrivate::ExecutionState &execution, const QString &path, quint64 handleMutationSerial) {
            for (auto it = execution.invalidatedPaths.cbegin(); it != execution.invalidatedPaths.cend(); ++it) {
                if (it.value() > handleMutationSerial && Internal::isPathWithin(it.key(), path)) {
                    return true;
                }
            }
            return false;
        }

        bool readAll(QFile *file, const std::optional<qint64> &maximumBytes, QByteArray *result, FileSystemError *error, const QString &operation, const QString &path) {
            if (maximumBytes && *maximumBytes < 0) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, operation, nativePath(path), FileSystemAccessInterface::tr("The maximum byte count must not be negative."));
                return false;
            }
            if (maximumBytes && file->size() > *maximumBytes) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::TooLarge, operation, nativePath(path), FileSystemAccessInterface::tr("The file is larger than the requested maximum byte count."));
                return false;
            }
            const auto bytesToRead = maximumBytes && *maximumBytes < std::numeric_limits<qint64>::max() ? *maximumBytes + 1 : -1;
            auto data = bytesToRead < 0 ? file->readAll() : file->read(bytesToRead);
            if (file->error() != QFileDevice::NoError) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, errorCodeForFileError(file->error()), operation, nativePath(path), file->errorString());
                return false;
            }
            if (maximumBytes && data.size() > *maximumBytes) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::TooLarge, operation, nativePath(path), FileSystemAccessInterface::tr("The file is larger than the requested maximum byte count."));
                return false;
            }
            *result = std::move(data);
            return true;
        }

        bool encodeText(const QString &text, const QString &encoding, QByteArray *result, FileSystemError *error, const QString &path) {
            QStringEncoder encoder(encoding);
            if (!encoder.isValid()) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Encoding, QStringLiteral("write"), nativePath(path), FileSystemAccessInterface::tr("Unsupported text encoding: %1").arg(encoding));
                return false;
            }
            *result = encoder(text);
            if (encoder.hasError()) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Encoding, QStringLiteral("write"), nativePath(path), FileSystemAccessInterface::tr("The text could not be encoded as %1.").arg(encoding));
                return false;
            }
            return true;
        }

        QString lastExtension(const QString &path) {
            const auto name = QFileInfo(path).fileName();
            const auto dotIndex = name.lastIndexOf(QLatin1Char('.'));
            return dotIndex <= 0 ? QString() : name.mid(dotIndex);
        }

        QString pathRoot(const QString &path) {
            const auto separatedPath = QDir::fromNativeSeparators(path);
#if defined(Q_OS_WIN)
            if (separatedPath.size() >= 3 && separatedPath.at(1) == QLatin1Char(':') && separatedPath.at(2) == QLatin1Char('/')) {
                const auto driveLetter = separatedPath.at(0).toUpper();
                if (driveLetter >= QLatin1Char('A') && driveLetter <= QLatin1Char('Z')) {
                    return QDir::toNativeSeparators(separatedPath.left(3));
                }
            }

            qsizetype serverStart = 2;
            if (separatedPath.startsWith(QStringLiteral("//?/UNC/"), Qt::CaseInsensitive)) {
                serverStart = 8;
            }
            if (separatedPath.startsWith(QStringLiteral("//"))) {
                const auto serverEnd = separatedPath.indexOf(QLatin1Char('/'), serverStart);
                if (serverEnd > serverStart) {
                    const auto shareEnd = separatedPath.indexOf(QLatin1Char('/'), serverEnd + 1);
                    if (shareEnd > serverEnd + 1) {
                        return QDir::toNativeSeparators(separatedPath.left(shareEnd + 1));
                    }
                    if (shareEnd < 0 && serverEnd + 1 < separatedPath.size()) {
                        return QDir::toNativeSeparators(separatedPath + QLatin1Char('/'));
                    }
                }
            }
            return separatedPath.startsWith(QLatin1Char('/')) ? QString(QDir::separator()) : QString();
#else
            return separatedPath.startsWith(QLatin1Char('/')) ? QStringLiteral("/") : QString();
#endif
        }

    }

    FileSystemAccessInterfacePrivate::FileSystemAccessInterfacePrivate(FileSystemAccessInterface *q) : q_ptr(q) {
    }

    FileSystemAccessInterfacePrivate::~FileSystemAccessInterfacePrivate() {
        if (currentExecution && currentExecution->token) {
            finalizeExecution(currentExecution->token->serial);
        }
    }

    void FileSystemAccessInterfacePrivate::beginExecution(ScriptExecutionContext *context) {
        if (currentExecution) {
            finalizeExecution(currentExecution->token->serial);
        }
        if (!context || !context->engine() || !context->script() || !context->action()) {
            return;
        }
        auto execution = std::make_shared<ExecutionState>();
        execution->token = QSharedPointer<Internal::FileSystemExecutionToken>::create();
        execution->token->active = true;
        execution->token->serial = ++nextExecutionSerial;
        execution->token->engine = context->engine();
        execution->token->context = context;
        execution->token->action = context->action();
        execution->scriptId = context->script()->metadata().id();
        execution->scriptRoot = context->script()->d_func()->rootPath;
        currentExecution = std::move(execution);
        qCDebug(lcFileSystemAccess) << "Started file-system execution scope" << currentExecution->scriptId << currentExecution->token->serial;
    }

    void FileSystemAccessInterfacePrivate::scheduleEndExecution(ScriptExecutionContext *context) {
        const auto execution = executionForContext(context);
        if (!execution || execution->cleanupScheduled) {
            return;
        }
        execution->cleanupScheduled = true;
        const auto serial = execution->token->serial;
        QMetaObject::invokeMethod(q_ptr, [this, serial] {
            if (!currentExecution || currentExecution->token->serial != serial) {
                return;
            }
            const auto context = currentExecution->token->context.data();
            if (context && context->action()) {
                return;
            }
            finalizeExecution(serial);
        }, Qt::QueuedConnection);
    }

    void FileSystemAccessInterfacePrivate::finalizeExecution(quint64 serial) {
        if (!currentExecution || currentExecution->token->serial != serial) {
            return;
        }
        currentExecution->token->active = false;
        if (currentExecution->temporaryDirectory) {
            const auto path = currentExecution->temporaryDirectoryPath;
            if (!currentExecution->temporaryDirectory->remove()) {
                qCWarning(lcFileSystemAccess) << "Failed to remove script temporary directory" << path;
            } else {
                qCDebug(lcFileSystemAccess) << "Removed script temporary directory" << path;
            }
        }
        qCDebug(lcFileSystemAccess) << "Finished file-system execution scope" << currentExecution->scriptId << serial;
        currentExecution.reset();
    }

    std::shared_ptr<FileSystemAccessInterfacePrivate::ExecutionState> FileSystemAccessInterfacePrivate::executionForContext(ScriptExecutionContext *context) const {
        if (!currentExecution || !currentExecution->token || !currentExecution->token->active || !context || currentExecution->token->context != context || currentExecution->token->engine != context->engine() || currentExecution->token->action != context->action() || !context->action()) {
            return {};
        }
        return currentExecution;
    }

    bool FileSystemAccessInterfacePrivate::accessCovers(FileAccessMode available, FileAccessMode requested) {
        return (accessMask(available) & accessMask(requested)) == accessMask(requested);
    }

    void FileSystemAccessInterfacePrivate::setPermissionError(FileSystemError *error, const QString &path, const QString &message) {
        if (!error) {
            return;
        }
        error->kind = FileSystemErrorKind::PermissionDenied;
        error->code = FileSystemErrorCode::None;
        error->message = message;
        error->path = path;
    }

    void FileSystemAccessInterfacePrivate::setFileSystemError(FileSystemError *error, FileSystemErrorCode code, const QString &operation, const QString &path, const QString &message, const QString &destinationPath) {
        if (!error) {
            return;
        }
        error->kind = FileSystemErrorKind::FileSystem;
        error->code = code;
        error->message = message;
        error->operation = operation;
        error->path = path;
        error->destinationPath = destinationPath;
    }

    bool FileSystemAccessInterfacePrivate::resolvePath(ScriptExecutionContext *context, const QString &path, QString *result, FileSystemError *error, const QString &operation) const {
        const auto script = context ? context->script() : nullptr;
        const auto rootPath = script ? script->d_func()->rootPath : QString();
        QString errorMessage;
        if (!Internal::resolveFileSystemPath(path, rootPath, result, &errorMessage)) {
            setFileSystemError(error, FileSystemErrorCode::InvalidPath, operation, path, errorMessage);
            return false;
        }
        return true;
    }

    bool FileSystemAccessInterfacePrivate::normalizePattern(ScriptExecutionContext *context, const QString &pattern, QString *result, FileSystemError *error) const {
        const auto script = context ? context->script() : nullptr;
        const auto rootPath = script ? script->d_func()->rootPath : QString();
        QString errorMessage;
        if (!Internal::normalizeFileSystemPattern(pattern, rootPath, result, &errorMessage)) {
            setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("permission"), pattern, errorMessage);
            return false;
        }
        return true;
    }

    bool FileSystemAccessInterfacePrivate::permissionCoversPath(const ExecutionState &execution, const QString &path, FileAccessMode access) const {
        const auto sessionIt = sessionPermissions.constFind(execution.scriptId);
        if (sessionIt != sessionPermissions.cend() && sessionIt->fullAccess) {
            return true;
        }
        auto availableMask = 0;
        auto collect = [&path, &availableMask](const QVector<PermissionGrant> &grants) {
            for (const auto &grant : grants) {
                if (Internal::matchesFileSystemPattern(path, grant.pattern)) {
                    availableMask |= accessMask(grant.access);
                }
            }
        };
        collect(execution.grants);
        if (sessionIt != sessionPermissions.cend()) {
            collect(sessionIt->grants);
        }
        return (availableMask & accessMask(access)) == accessMask(access);
    }

    bool FileSystemAccessInterfacePrivate::permissionCoversPattern(const ExecutionState &execution, const QString &pattern, FileAccessMode access) const {
        const auto sessionIt = sessionPermissions.constFind(execution.scriptId);
        if (sessionIt != sessionPermissions.cend() && sessionIt->fullAccess) {
            return true;
        }
        auto coveredMask = 0;
        auto collect = [&pattern, &coveredMask](const QVector<PermissionGrant> &grants) {
            for (const auto &grant : grants) {
                if (grant.pattern == pattern || (!Internal::fileSystemPatternHasMagic(pattern) && Internal::matchesFileSystemPattern(pattern, grant.pattern)) || Internal::recursivePatternContains(grant.pattern, pattern)) {
                    coveredMask |= accessMask(grant.access);
                }
            }
        };
        collect(execution.grants);
        if (sessionIt != sessionPermissions.cend()) {
            collect(sessionIt->grants);
        }
        return (coveredMask & accessMask(access)) == accessMask(access);
    }

    bool FileSystemAccessInterfacePrivate::canonicalPermissionCheck(const ExecutionState &execution, const QString &path, FileAccessMode access, bool followTarget, FileSystemError *error, const QString &operation) const {
        if (!permissionCoversPath(execution, path, access)) {
            setPermissionError(error, nativePath(path), FileSystemAccessInterface::tr("The script does not have permission to access this path."));
            return false;
        }
        if (!followTarget) {
            return true;
        }
        const auto resolvedPath = canonicalOrDerivedPath(path);
        if (!resolvedPath.isEmpty() && resolvedPath != path && !permissionCoversPath(execution, resolvedPath, access)) {
            setPermissionError(error, nativePath(resolvedPath), FileSystemAccessInterface::tr("The resolved path is outside the script's granted file-system permissions."));
            return false;
        }
        Q_UNUSED(operation)
        return true;
    }

    bool FileSystemAccessInterfacePrivate::handleCoversPath(const FileSystemHandle &handle, const QString &path) const {
        if (!handle.d) {
            return false;
        }
        if (handle.d->scopePath == path) {
            return true;
        }
        if (!Internal::isPathWithin(handle.d->scopePath, path)) {
            return false;
        }
        if (handle.d->scopeRecursive) {
            return true;
        }
        const auto relativePath = QDir(handle.d->scopePath).relativeFilePath(path);
        return !relativePath.contains(QLatin1Char('/')) && !relativePath.contains(QLatin1Char('\\'));
    }

    FileSystemHandle FileSystemAccessInterfacePrivate::createHandle(const std::shared_ptr<ExecutionState> &execution, FileSystemHandle::Kind kind, const QString &path, FileAccessMode access, bool recursive, bool intrinsicCapability, const QString &scopePath, bool scopeRecursive) const {
        if (!execution || !execution->token || !execution->token->active) {
            return {};
        }
        auto handlePrivate = new FileSystemHandlePrivate;
        handlePrivate->kind = kind;
        handlePrivate->path = Internal::normalizedFileSystemPath(path);
        handlePrivate->access = access;
        handlePrivate->recursive = recursive;
        handlePrivate->intrinsicCapability = intrinsicCapability;
        handlePrivate->scopePath = scopePath.isEmpty() ? handlePrivate->path : Internal::normalizedFileSystemPath(scopePath);
        handlePrivate->scopeRecursive = scopePath.isEmpty() ? recursive : scopeRecursive;
        handlePrivate->createdAtMutationSerial = execution->mutationSerial;
        handlePrivate->token = execution->token;
        return FileSystemHandle(handlePrivate);
    }

    FileSystemAccessInterface::FileSystemAccessInterface(QObject *parent)
        : QObject(parent), d_ptr(new FileSystemAccessInterfacePrivate(this)) {
        Q_ASSERT(!m_instance);
        m_instance = this;
    }

    FileSystemAccessInterface::~FileSystemAccessInterface() {
        m_instance = nullptr;
    }

    FileSystemAccessInterface *FileSystemAccessInterface::instance() {
        return m_instance;
    }

    FileSystemPermissionDecision FileSystemAccessInterface::requestPermission(ScriptExecutionContext *context, const FileSystemPermissionRequest &request, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("permission"), {}, tr("File-system permissions can only be requested while a script action is executing."));
            return FileSystemPermissionDecision::Failed;
        }
        if (request.reason.trimmed().isEmpty()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("permission"), request.pathPattern, tr("A file-system permission request must include a non-empty reason."));
            return FileSystemPermissionDecision::Failed;
        }

        QString pattern;
        if (!d->normalizePattern(context, request.pathPattern, &pattern, error)) {
            return FileSystemPermissionDecision::Failed;
        }
        if (d->permissionCoversPattern(*execution, pattern, request.access)) {
            qCDebug(lcFileSystemAccess) << "Reused existing file-system permission" << execution->scriptId << pattern;
            return FileSystemPermissionDecision::Allowed;
        }

        const auto script = context->script();
        auto parentWidget = context->windowInterface() ? context->windowInterface()->invisibleCentralWidget() : nullptr;
        Internal::FileSystemPermissionDialog dialog(script->metadata().name(), script->metadata().id(), nativePath(pattern), request.access, request.reason.trimmed(), parentWidget);
        dialog.exec();
        switch (dialog.decision()) {
            case Internal::FileSystemPermissionDialog::AllowOnce:
                execution->grants.append({pattern, request.access});
                qCInfo(lcFileSystemAccess) << "Granted one-time file-system permission" << execution->scriptId << pattern;
                return FileSystemPermissionDecision::Allowed;
            case Internal::FileSystemPermissionDialog::AlwaysAllow:
                d->sessionPermissions[execution->scriptId].grants.append({pattern, request.access});
                qCInfo(lcFileSystemAccess) << "Granted session file-system permission" << execution->scriptId << pattern;
                return FileSystemPermissionDecision::Allowed;
            case Internal::FileSystemPermissionDialog::AllowFullAccess:
                d->sessionPermissions[execution->scriptId].fullAccess = true;
                qCInfo(lcFileSystemAccess) << "Granted full session file-system access" << execution->scriptId;
                return FileSystemPermissionDecision::Allowed;
            case Internal::FileSystemPermissionDialog::Deny:
                qCWarning(lcFileSystemAccess) << "Denied file-system permission" << execution->scriptId << pattern;
                return FileSystemPermissionDecision::Denied;
        }
        return FileSystemPermissionDecision::Denied;
    }

    bool FileSystemAccessInterface::hasPermission(ScriptExecutionContext *context, const QString &path, FileAccessMode access, bool recursive) const {
        Q_D(const FileSystemAccessInterface);
        const auto execution = d->executionForContext(context);
        if (!execution) {
            return false;
        }
        QString resolvedPath;
        if (!d->resolvePath(context, path, &resolvedPath, nullptr, QStringLiteral("permission"))) {
            return false;
        }
        if (!recursive) {
            if (!d->permissionCoversPath(*execution, resolvedPath, access)) {
                return false;
            }
            const auto canonicalPath = canonicalOrDerivedPath(resolvedPath);
            return canonicalPath.isEmpty() || canonicalPath == resolvedPath || d->permissionCoversPath(*execution, canonicalPath, access);
        }
        if (!d->permissionCoversPattern(*execution, resolvedPath + QStringLiteral("/**"), access)) {
            return false;
        }
        const auto canonicalPath = canonicalOrDerivedPath(resolvedPath);
        return canonicalPath.isEmpty() || canonicalPath == resolvedPath || d->permissionCoversPattern(*execution, canonicalPath + QStringLiteral("/**"), access);
    }

    FileSystemHandle FileSystemAccessInterface::getFile(ScriptExecutionContext *context, const QString &path, FileAccessMode access, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("getFile"), path, tr("File handles can only be created while a script action is executing."));
            return {};
        }
        QString resolvedPath;
        if (!d->resolvePath(context, path, &resolvedPath, error, QStringLiteral("getFile")) || !d->canonicalPermissionCheck(*execution, resolvedPath, access, true, error, QStringLiteral("getFile"))) {
            return {};
        }
        QFileInfo fileInfo(resolvedPath);
        if (fileInfo.isSymbolicLink() && fileInfo.canonicalFilePath().isEmpty()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("getFile"), nativePath(resolvedPath), tr("The symbolic link target does not exist."));
            return {};
        }
        if (fileInfo.exists() && !fileInfo.isFile()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFile, QStringLiteral("getFile"), nativePath(resolvedPath), tr("The requested path is not a regular file."));
            return {};
        }
        if (!fileInfo.exists() && access == FileAccessMode::Read) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("getFile"), nativePath(resolvedPath), tr("The requested file does not exist."));
            return {};
        }
        const auto handlePath = fileInfo.exists() ? Internal::normalizedFileSystemPath(fileInfo.canonicalFilePath()) : canonicalOrDerivedPath(resolvedPath);
        return d->createHandle(execution, FileSystemHandle::File, handlePath.isEmpty() ? resolvedPath : handlePath, access, false, false);
    }

    FileSystemHandle FileSystemAccessInterface::getDirectory(ScriptExecutionContext *context, const QString &path, FileAccessMode access, bool recursive, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("getDirectory"), path, tr("Directory handles can only be created while a script action is executing."));
            return {};
        }
        QString resolvedPath;
        if (!d->resolvePath(context, path, &resolvedPath, error, QStringLiteral("getDirectory"))) {
            return {};
        }
        if (recursive) {
            const auto recursivePattern = resolvedPath + QStringLiteral("/**");
            if (!d->permissionCoversPattern(*execution, recursivePattern, access)) {
                FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(resolvedPath), tr("The script does not have recursive permission for this directory."));
                return {};
            }
        } else if (!d->canonicalPermissionCheck(*execution, resolvedPath, access, true, error, QStringLiteral("getDirectory"))) {
            return {};
        }
        QFileInfo fileInfo(resolvedPath);
        if (!fileInfo.exists()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("getDirectory"), nativePath(resolvedPath), tr("The requested directory does not exist."));
            return {};
        }
        if (!fileInfo.isDir()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotDirectory, QStringLiteral("getDirectory"), nativePath(resolvedPath), tr("The requested path is not a directory."));
            return {};
        }
        const auto canonicalPath = Internal::normalizedFileSystemPath(fileInfo.canonicalFilePath());
        if (!d->canonicalPermissionCheck(*execution, canonicalPath, access, false, error, QStringLiteral("getDirectory"))) {
            return {};
        }
        if (recursive && !d->permissionCoversPattern(*execution, canonicalPath + QStringLiteral("/**"), access)) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(canonicalPath), tr("The script does not have recursive permission for the resolved directory."));
            return {};
        }
        return d->createHandle(execution, FileSystemHandle::Directory, canonicalPath, access, recursive, false);
    }

    FileSystemHandle FileSystemAccessInterface::scriptPackage(ScriptExecutionContext *context, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution || !context->script()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("scriptPackage"), {}, tr("The script package is only available while a script action is executing."));
            return {};
        }
        const auto packagePath = context->script()->filePath();
        if (packagePath.isEmpty()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotSupported, QStringLiteral("scriptPackage"), {}, tr("Built-in scripts do not have a file-system package."));
            return {};
        }
        QFileInfo packageInfo(packagePath);
        const auto canonicalPath = Internal::normalizedFileSystemPath(packageInfo.canonicalFilePath());
        if (packageInfo.isFile()) {
            return d->createHandle(execution, FileSystemHandle::File, canonicalPath, FileAccessMode::Read, false, true);
        }
        if (packageInfo.isDir()) {
            return d->createHandle(execution, FileSystemHandle::Directory, canonicalPath, FileAccessMode::Read, true, true);
        }
        FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("scriptPackage"), packagePath, tr("The script package no longer exists."));
        return {};
    }

    FileSystemHandle FileSystemAccessInterface::dataDirectory(ScriptExecutionContext *context, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("dataDirectory"), {}, tr("The script data directory is only available while a script action is executing."));
            return {};
        }
        const auto path = QDir(Internal::BatchProcessSettings::scriptDataDirectory()).filePath(Internal::safeScriptDirectoryName(execution->scriptId));
        if (!QDir().mkpath(path)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Io, QStringLiteral("dataDirectory"), nativePath(path), tr("Failed to create the script data directory."));
            return {};
        }
        const auto canonicalPath = Internal::normalizedFileSystemPath(QFileInfo(path).canonicalFilePath());
        if (canonicalPath.isEmpty()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Io, QStringLiteral("dataDirectory"), nativePath(path), tr("The script data directory could not be resolved."));
            return {};
        }
        return d->createHandle(execution, FileSystemHandle::Directory, canonicalPath, FileAccessMode::ReadWrite, true, true);
    }

    FileSystemHandle FileSystemAccessInterface::temporaryDirectory(ScriptExecutionContext *context, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("temporaryDirectory"), {}, tr("The temporary directory is only available while a script action is executing."));
            return {};
        }
        if (!execution->temporaryDirectory) {
            const auto applicationTemporaryPath = Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::TempData);
            if (applicationTemporaryPath.isEmpty()) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Io, QStringLiteral("temporaryDirectory"), {}, tr("The application temporary directory is unavailable."));
                return {};
            }
            const auto rootPath = QDir(applicationTemporaryPath).filePath(QStringLiteral("BatchProcess/%1").arg(Internal::safeScriptDirectoryName(execution->scriptId)));
            if (!QDir().mkpath(rootPath)) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Io, QStringLiteral("temporaryDirectory"), nativePath(rootPath), tr("Failed to create the script temporary-directory root."));
                return {};
            }
            execution->temporaryDirectory = std::make_unique<QTemporaryDir>(QDir(rootPath).filePath(QStringLiteral("action-XXXXXX")));
            if (!execution->temporaryDirectory->isValid()) {
                execution->temporaryDirectory.reset();
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Io, QStringLiteral("temporaryDirectory"), nativePath(rootPath), tr("Failed to create a temporary directory for the current action."));
                return {};
            }
            execution->temporaryDirectory->setAutoRemove(false);
            execution->temporaryDirectoryPath = Internal::normalizedFileSystemPath(QFileInfo(execution->temporaryDirectory->path()).canonicalFilePath());
            if (execution->temporaryDirectoryPath.isEmpty()) {
                execution->temporaryDirectory->remove();
                execution->temporaryDirectory.reset();
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Io, QStringLiteral("temporaryDirectory"), nativePath(rootPath), tr("The action temporary directory could not be resolved."));
                return {};
            }
            qCDebug(lcFileSystemAccess) << "Created script temporary directory" << execution->temporaryDirectoryPath;
        }
        return d->createHandle(execution, FileSystemHandle::Directory, execution->temporaryDirectoryPath, FileAccessMode::ReadWrite, true, true);
    }

    FileSystemHandle FileSystemAccessInterface::createAuthorizedFileHandle(ScriptExecutionContext *context, const QString &path, FileAccessMode access, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("authorizeFile"), path, tr("Authorized handles can only be created while a script action is executing."));
            return {};
        }
        QString resolvedPath;
        if (!d->resolvePath(context, path, &resolvedPath, error, QStringLiteral("authorizeFile"))) {
            return {};
        }
        QFileInfo fileInfo(resolvedPath);
        if (fileInfo.exists() && !fileInfo.isFile()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFile, QStringLiteral("authorizeFile"), nativePath(resolvedPath), tr("The authorized path is not a regular file."));
            return {};
        }
        if (!fileInfo.exists() && access == FileAccessMode::Read) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("authorizeFile"), nativePath(resolvedPath), tr("The authorized file does not exist."));
            return {};
        }
        const auto handlePath = fileInfo.exists() ? Internal::normalizedFileSystemPath(fileInfo.canonicalFilePath()) : canonicalOrDerivedPath(resolvedPath);
        return d->createHandle(execution, FileSystemHandle::File, handlePath.isEmpty() ? resolvedPath : handlePath, access, false, true);
    }

    FileSystemHandle FileSystemAccessInterface::createAuthorizedDirectoryHandle(ScriptExecutionContext *context, const QString &path, FileAccessMode access, bool recursive, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("authorizeDirectory"), path, tr("Authorized handles can only be created while a script action is executing."));
            return {};
        }
        QString resolvedPath;
        if (!d->resolvePath(context, path, &resolvedPath, error, QStringLiteral("authorizeDirectory"))) {
            return {};
        }
        QFileInfo fileInfo(resolvedPath);
        if (!fileInfo.exists()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("authorizeDirectory"), nativePath(resolvedPath), tr("The authorized directory does not exist."));
            return {};
        }
        if (!fileInfo.isDir()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotDirectory, QStringLiteral("authorizeDirectory"), nativePath(resolvedPath), tr("The authorized path is not a directory."));
            return {};
        }
        return d->createHandle(execution, FileSystemHandle::Directory, Internal::normalizedFileSystemPath(fileInfo.canonicalFilePath()), access, recursive, true);
    }

    bool FileSystemAccessInterface::validateHandle(ScriptExecutionContext *context, const FileSystemHandle &handle, FileAccessMode access, FileSystemHandle::Kind expectedKind, FileSystemError *error) const {
        Q_D(const FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution || !handle.d || handle.d->token != execution->token || !handle.isValid()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), handle.path(), tr("The file-system handle is no longer valid for the current script action."));
            return false;
        }
        if (expectedKind != FileSystemHandle::Invalid && handle.d->kind != expectedKind) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, expectedKind == FileSystemHandle::File ? FileSystemErrorCode::NotFile : FileSystemErrorCode::NotDirectory, QStringLiteral("handle"), handle.path(), tr("The file-system handle has the wrong kind for this operation."));
            return false;
        }
        if (!FileSystemAccessInterfacePrivate::accessCovers(handle.d->access, access)) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, handle.path(), tr("The file-system handle does not provide the requested access mode."));
            return false;
        }
        if (pathIsInvalidated(*execution, handle.d->path, handle.d->createdAtMutationSerial)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), handle.path(), tr("The file-system handle refers to an entry that was moved, removed, or replaced."));
            return false;
        }
        const auto resolvedHandlePath = canonicalOrDerivedPath(handle.d->path);
        if (!resolvedHandlePath.isEmpty() && !d->handleCoversPath(handle, resolvedHandlePath)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), handle.path(), tr("The file-system handle now resolves outside its authorized capability scope."));
            return false;
        }
        const QFileInfo fileInfo(handle.d->path);
        if (handle.d->kind == FileSystemHandle::Directory && (!fileInfo.exists() || !fileInfo.isDir())) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), handle.path(), tr("The directory represented by this handle no longer exists."));
            return false;
        }
        if (handle.d->kind == FileSystemHandle::File && fileInfo.exists() && !fileInfo.isFile()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), handle.path(), tr("The path represented by this handle is no longer a regular file."));
            return false;
        }
        if (fileInfo.exists()) {
            const auto canonicalPath = Internal::normalizedFileSystemPath(fileInfo.canonicalFilePath());
            if (!canonicalPath.isEmpty() && canonicalPath != handle.d->path) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), handle.path(), tr("The path represented by this handle now resolves to a different entry."));
                return false;
            }
        }
        return true;
    }

    bool FileSystemAccessInterface::stat(ScriptExecutionContext *context, const QString &path, const FileSystemStatOptions &options, FileSystemStat *result, FileSystemError *error) const {
        Q_D(const FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        if (!result) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("stat"), path, tr("No destination was provided for the file status."));
            return false;
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("stat"), path, tr("File status can only be queried while a script action is executing."));
            return false;
        }
        QString resolvedPath;
        if (!d->resolvePath(context, path, &resolvedPath, error, QStringLiteral("stat"))) {
            return false;
        }
        const auto readAllowed = d->permissionCoversPath(*execution, resolvedPath, FileAccessMode::Read);
        const auto writeAllowed = d->permissionCoversPath(*execution, resolvedPath, FileAccessMode::Write);
        if (!readAllowed && !writeAllowed) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(resolvedPath), tr("The script does not have permission to query this path."));
            return false;
        }
        const auto checkingAccess = readAllowed ? FileAccessMode::Read : FileAccessMode::Write;
        if (!d->canonicalPermissionCheck(*execution, resolvedPath, checkingAccess, options.followSymbolicLinks, error, QStringLiteral("stat"))) {
            return false;
        }

        FileSystemStat stat;
        stat.path = nativePath(resolvedPath);
        QFileInfo linkInfo(resolvedPath);
        const auto isLink = linkInfo.isSymbolicLink() || linkInfo.isJunction();
        stat.exists = linkInfo.exists() || isLink;
        if (!stat.exists) {
            *result = std::move(stat);
            return true;
        }
        stat.kind = entryKind(linkInfo);
        stat.hidden = linkInfo.isHidden();
        if (isLink) {
            const auto target = linkInfo.isJunction() ? linkInfo.junctionTarget() : linkInfo.symLinkTarget();
            stat.symbolicLinkTarget = target.isEmpty() ? QString() : nativePath(QFileInfo(target).absoluteFilePath());
            if (!options.followSymbolicLinks) {
                *result = std::move(stat);
                return true;
            }
        }

        const auto canonicalPath = Internal::normalizedFileSystemPath(linkInfo.canonicalFilePath());
        if (canonicalPath.isEmpty()) {
            *result = std::move(stat);
            return true;
        }
        stat.canonicalPath = nativePath(canonicalPath);
        const QFileInfo targetInfo(canonicalPath);
        stat.resolvedKind = resolvedEntryKind(targetInfo);
        if (targetInfo.isFile()) {
            stat.size = targetInfo.size();
        }
        stat.createdAt = targetInfo.birthTime();
        stat.modifiedAt = targetInfo.lastModified();
        stat.accessedAt = targetInfo.lastRead();
        stat.metadataChangedAt = targetInfo.metadataChangeTime();
        stat.readable = targetInfo.isReadable();
        stat.writable = targetInfo.isWritable();
        stat.executable = targetInfo.isExecutable();
        *result = std::move(stat);
        return true;
    }

    bool FileSystemAccessInterface::stat(ScriptExecutionContext *context, const FileSystemHandle &handle, const FileSystemStatOptions &options, FileSystemStat *result, FileSystemError *error) const {
        if (!validateHandle(context, handle, handle.access(), FileSystemHandle::Invalid, error)) {
            return false;
        }
        if (!result) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("stat"), handle.path(), tr("No destination was provided for the file status."));
            return false;
        }
        FileSystemStat stat;
        stat.path = handle.path();
        QFileInfo linkInfo(handle.d->path);
        const auto isLink = linkInfo.isSymbolicLink() || linkInfo.isJunction();
        stat.exists = linkInfo.exists() || isLink;
        if (!stat.exists) {
            *result = std::move(stat);
            return true;
        }
        stat.kind = entryKind(linkInfo);
        stat.hidden = linkInfo.isHidden();
        if (isLink) {
            const auto target = linkInfo.isJunction() ? linkInfo.junctionTarget() : linkInfo.symLinkTarget();
            stat.symbolicLinkTarget = target.isEmpty() ? QString() : nativePath(QFileInfo(target).absoluteFilePath());
            if (!options.followSymbolicLinks) {
                *result = std::move(stat);
                return true;
            }
        }
        const auto canonicalPath = Internal::normalizedFileSystemPath(linkInfo.canonicalFilePath());
        if (!canonicalPath.isEmpty()) {
            stat.canonicalPath = nativePath(canonicalPath);
            const QFileInfo targetInfo(canonicalPath);
            stat.resolvedKind = resolvedEntryKind(targetInfo);
            if (targetInfo.isFile()) {
                stat.size = targetInfo.size();
            }
            stat.createdAt = targetInfo.birthTime();
            stat.modifiedAt = targetInfo.lastModified();
            stat.accessedAt = targetInfo.lastRead();
            stat.metadataChangedAt = targetInfo.metadataChangeTime();
            stat.readable = targetInfo.isReadable();
            stat.writable = targetInfo.isWritable();
            stat.executable = targetInfo.isExecutable();
        }
        *result = std::move(stat);
        return true;
    }

    bool FileSystemAccessInterface::readBytes(ScriptExecutionContext *context, const FileSystemHandle &handle, const FileBinaryReadOptions &options, QByteArray *result, FileSystemError *error) const {
        if (error) {
            error->clear();
        }
        if (!result) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("read"), handle.path(), tr("No destination was provided for the file contents."));
            return false;
        }
        if (!validateHandle(context, handle, FileAccessMode::Read, FileSystemHandle::File, error)) {
            return false;
        }
        QFile file(handle.d->path);
        if (!file.open(QIODevice::ReadOnly)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, file.exists() ? errorCodeForFileError(file.error()) : FileSystemErrorCode::NotFound, QStringLiteral("read"), handle.path(), file.errorString());
            return false;
        }
        return readAll(&file, options.maximumBytes, result, error, QStringLiteral("read"), handle.d->path);
    }

    bool FileSystemAccessInterface::readText(ScriptExecutionContext *context, const FileSystemHandle &handle, const FileTextReadOptions &options, QString *result, FileSystemError *error) const {
        if (error) {
            error->clear();
        }
        if (!result) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("read"), handle.path(), tr("No destination was provided for the decoded text."));
            return false;
        }
        QByteArray bytes;
        if (!readBytes(context, handle, {options.maximumBytes}, &bytes, error)) {
            return false;
        }
        QStringDecoder decoder(options.encoding);
        if (!decoder.isValid()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Encoding, QStringLiteral("read"), handle.path(), tr("Unsupported text encoding: %1").arg(options.encoding));
            return false;
        }
        auto text = decoder(bytes);
        if (decoder.hasError()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Encoding, QStringLiteral("read"), handle.path(), tr("The file contains invalid %1 text.").arg(options.encoding));
            return false;
        }
        *result = std::move(text);
        return true;
    }

    bool FileSystemAccessInterface::writeBytes(ScriptExecutionContext *context, const FileSystemHandle &handle, QByteArrayView data, const FileBinaryWriteOptions &options, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        if (!validateHandle(context, handle, FileAccessMode::Write, FileSystemHandle::File, error)) {
            return false;
        }
        const auto atomic = options.atomic.value_or(options.disposition == FileWriteDisposition::Replace);
        if (atomic && options.disposition != FileWriteDisposition::Replace) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("write"), handle.path(), tr("Atomic writing is only available with replace disposition."));
            return false;
        }

        const QFileInfo targetInfo(handle.d->path);
        if (options.disposition == FileWriteDisposition::CreateNew && (targetInfo.exists() || targetInfo.isSymbolicLink())) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::AlreadyExists, QStringLiteral("write"), handle.path(), tr("The destination file already exists."));
            return false;
        }
        if (targetInfo.exists() && !targetInfo.isFile()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFile, QStringLiteral("write"), handle.path(), tr("The destination path is not a regular file."));
            return false;
        }

        const auto parentPath = Internal::normalizedFileSystemPath(targetInfo.absolutePath());
        if (!QFileInfo(parentPath).isDir()) {
            if (!options.createParents) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotDirectory, QStringLiteral("write"), nativePath(parentPath), tr("The destination parent directory does not exist."));
                return false;
            }
            if (!d->handleCoversPath(handle, parentPath)) {
                FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(parentPath), tr("The file handle does not authorize creating the destination parent directories."));
                return false;
            }
            if (!QDir().mkpath(parentPath)) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Io, QStringLiteral("write"), nativePath(parentPath), tr("Failed to create the destination parent directories."));
                return false;
            }
        }

        auto writeToDevice = [data, error, &handle](QIODevice *device) {
            const auto written = device->write(data.data(), data.size());
            if (written != data.size()) {
                const auto fileDevice = qobject_cast<QFileDevice *>(device);
                FileSystemAccessInterfacePrivate::setFileSystemError(error, fileDevice ? errorCodeForFileError(fileDevice->error()) : FileSystemErrorCode::Io, QStringLiteral("write"), handle.path(), fileDevice ? fileDevice->errorString() : FileSystemAccessInterface::tr("Failed to write all requested bytes."));
                return false;
            }
            return true;
        };

        if (atomic) {
            QSaveFile file(handle.d->path);
            file.setDirectWriteFallback(false);
            if (!file.open(QIODevice::WriteOnly)) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, errorCodeForFileError(file.error()), QStringLiteral("write"), handle.path(), file.errorString());
                return false;
            }
            if (!writeToDevice(&file)) {
                file.cancelWriting();
                return false;
            }
            if (!file.commit()) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, errorCodeForFileError(file.error()), QStringLiteral("write"), handle.path(), file.errorString());
                return false;
            }
            return true;
        }

        QIODevice::OpenMode openMode = QIODevice::WriteOnly;
        if (options.disposition == FileWriteDisposition::Append) {
            openMode |= QIODevice::Append;
        } else if (options.disposition == FileWriteDisposition::CreateNew) {
            openMode |= QIODevice::NewOnly;
        } else {
            openMode |= QIODevice::Truncate;
        }
        QFile file(handle.d->path);
        if (!file.open(openMode)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, file.exists() && options.disposition == FileWriteDisposition::CreateNew ? FileSystemErrorCode::AlreadyExists : errorCodeForFileError(file.error()), QStringLiteral("write"), handle.path(), file.errorString());
            return false;
        }
        if (!writeToDevice(&file) || !file.flush()) {
            if (!error || !error->hasError()) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, errorCodeForFileError(file.error()), QStringLiteral("write"), handle.path(), file.errorString());
            }
            return false;
        }
        return true;
    }

    bool FileSystemAccessInterface::writeText(ScriptExecutionContext *context, const FileSystemHandle &handle, const QString &text, const FileTextWriteOptions &options, FileSystemError *error) {
        if (error) {
            error->clear();
        }
        QByteArray bytes;
        if (!encodeText(text, options.encoding, &bytes, error, handle.path())) {
            return false;
        }
        return writeBytes(context, handle, bytes, {options.disposition, options.atomic, options.createParents}, error);
    }

    bool FileSystemAccessInterface::entries(ScriptExecutionContext *context, const FileSystemHandle &handle, const FileSystemDirectoryListOptions &options, QList<FileSystemDirectoryEntry> *result, FileSystemError *error) const {
        if (error) {
            error->clear();
        }
        if (!result) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("list"), handle.path(), tr("No destination was provided for the directory entries."));
            return false;
        }
        if (!validateHandle(context, handle, FileAccessMode::Read, FileSystemHandle::Directory, error)) {
            return false;
        }
        if (options.recursive && !handle.d->recursive) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, handle.path(), tr("This directory handle does not allow recursive enumeration."));
            return false;
        }

        for (const auto &filter : options.nameFilters) {
            QString filterError;
            Internal::matchesFileSystemPattern(QStringLiteral("probe"), filter, &filterError);
            if (!filterError.isEmpty()) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("list"), handle.path(), filterError);
                return false;
            }
        }

        QList<FileSystemDirectoryEntry> entries;
        QStringList pendingDirectories{handle.d->path};
        while (!pendingDirectories.isEmpty()) {
            const auto directoryPath = pendingDirectories.takeFirst();
            const QDir directory(directoryPath);
            const auto fileInfos = directory.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDir::NoSort);
            for (const auto &fileInfo : fileInfos) {
                const auto absolutePath = Internal::normalizedFileSystemPath(fileInfo.absoluteFilePath());
                const auto kind = entryKind(fileInfo);
                const auto isLink = kind == FileSystemEntryKind::SymbolicLink;
                const auto resolvedKind = isLink ? resolvedEntryKind(QFileInfo(fileInfo.canonicalFilePath())) : resolvedEntryKind(fileInfo);
                auto relativePath = QDir(handle.d->path).relativeFilePath(absolutePath);
                relativePath = Internal::normalizedFileSystemPath(relativePath);

                bool filterMatches = options.nameFilters.isEmpty();
                for (const auto &filter : options.nameFilters) {
                    if (Internal::matchesFileSystemPattern(relativePath, filter)) {
                        filterMatches = true;
                        break;
                    }
                }
                const auto resolvedAsFile = resolvedKind == FileSystemEntryKind::File;
                const auto resolvedAsDirectory = resolvedKind == FileSystemEntryKind::Directory;
                const auto includeByKind = (resolvedAsFile && options.files) || (resolvedAsDirectory && options.directories) || (!resolvedAsFile && !resolvedAsDirectory && (options.files || options.directories));
                if (filterMatches && includeByKind) {
                    entries.append({relativePath, nativePath(absolutePath), kind, resolvedKind});
                }
                if (options.recursive && !isLink && fileInfo.isDir() && Internal::isPathWithin(handle.d->path, absolutePath)) {
                    pendingDirectories.append(absolutePath);
                }
            }
        }

        std::ranges::stable_sort(entries, [](const FileSystemDirectoryEntry &left, const FileSystemDirectoryEntry &right) {
#if defined(Q_OS_WIN)
            const auto insensitive = QString::compare(left.relativePath, right.relativePath, Qt::CaseInsensitive);
            return insensitive == 0 ? left.relativePath < right.relativePath : insensitive < 0;
#else
            return left.relativePath < right.relativePath;
#endif
        });
        *result = std::move(entries);
        return true;
    }

    FileSystemHandle FileSystemAccessInterface::childFile(ScriptExecutionContext *context, const FileSystemHandle &directory, const QString &relativePath, FileAccessMode access, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        if (!validateHandle(context, directory, access, FileSystemHandle::Directory, error)) {
            return {};
        }
        if (relativePath.trimmed().isEmpty() || QDir::isAbsolutePath(relativePath) || relativePath.contains(QChar::Null)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("getFile"), relativePath, tr("A child file path must be a non-empty relative path."));
            return {};
        }
        const auto cleanRelativePath = Internal::normalizedFileSystemPath(relativePath);
        if (cleanRelativePath == QStringLiteral("..") || cleanRelativePath.startsWith(QStringLiteral("../"))) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, relativePath, tr("The child file path escapes the directory handle."));
            return {};
        }
        if (!directory.d->recursive && (cleanRelativePath.contains(QLatin1Char('/')) || cleanRelativePath.contains(QLatin1Char('\\')))) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, relativePath, tr("This directory handle only allows direct child files."));
            return {};
        }
        const auto path = Internal::normalizedFileSystemPath(QDir(directory.d->path).filePath(cleanRelativePath));
        const auto canonicalPath = canonicalOrDerivedPath(path);
        if (!Internal::isPathWithin(directory.d->path, canonicalPath.isEmpty() ? path : canonicalPath)) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(path), tr("The child file resolves outside the directory handle."));
            return {};
        }
        QFileInfo fileInfo(path);
        if (fileInfo.exists() && !fileInfo.isFile()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFile, QStringLiteral("getFile"), nativePath(path), tr("The requested child is not a regular file."));
            return {};
        }
        if (!fileInfo.exists() && access == FileAccessMode::Read) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("getFile"), nativePath(path), tr("The requested child file does not exist."));
            return {};
        }
        const auto execution = d->executionForContext(context);
        return d->createHandle(execution, FileSystemHandle::File, canonicalPath.isEmpty() ? path : canonicalPath, access, false, directory.d->intrinsicCapability, directory.d->scopePath, directory.d->scopeRecursive);
    }

    FileSystemHandle FileSystemAccessInterface::childDirectory(ScriptExecutionContext *context, const FileSystemHandle &directory, const QString &relativePath, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        if (!validateHandle(context, directory, directory.access(), FileSystemHandle::Directory, error)) {
            return {};
        }
        if (relativePath.isEmpty()) {
            return directory;
        }
        if (!directory.d->recursive) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, relativePath, tr("This directory handle does not allow access to child directories."));
            return {};
        }
        if (QDir::isAbsolutePath(relativePath) || relativePath.contains(QChar::Null)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("getDirectory"), relativePath, tr("A child directory path must be relative."));
            return {};
        }
        const auto cleanRelativePath = Internal::normalizedFileSystemPath(relativePath);
        if (cleanRelativePath == QStringLiteral("..") || cleanRelativePath.startsWith(QStringLiteral("../"))) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, relativePath, tr("The child directory path escapes the directory handle."));
            return {};
        }
        const auto path = Internal::normalizedFileSystemPath(QDir(directory.d->path).filePath(cleanRelativePath));
        QFileInfo fileInfo(path);
        if (!fileInfo.exists()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("getDirectory"), nativePath(path), tr("The requested child directory does not exist."));
            return {};
        }
        if (!fileInfo.isDir()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotDirectory, QStringLiteral("getDirectory"), nativePath(path), tr("The requested child is not a directory."));
            return {};
        }
        const auto canonicalPath = Internal::normalizedFileSystemPath(fileInfo.canonicalFilePath());
        if (!Internal::isPathWithin(directory.d->path, canonicalPath)) {
            FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(canonicalPath), tr("The child directory resolves outside the directory handle."));
            return {};
        }
        const auto execution = d->executionForContext(context);
        return d->createHandle(execution, FileSystemHandle::Directory, canonicalPath, directory.access(), true, directory.d->intrinsicCapability, directory.d->scopePath, directory.d->scopeRecursive);
    }

    bool FileSystemAccessInterface::createDirectory(ScriptExecutionContext *context, const QString &path, const FileSystemCreateDirectoryOptions &options, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("mkdir"), path, tr("Directories can only be created while a script action is executing."));
            return false;
        }
        QString resolvedPath;
        if (!d->resolvePath(context, path, &resolvedPath, error, QStringLiteral("mkdir")) || !d->canonicalPermissionCheck(*execution, resolvedPath, FileAccessMode::Write, true, error, QStringLiteral("mkdir"))) {
            return false;
        }

        const QFileInfo targetInfo(resolvedPath);
        if (targetInfo.exists()) {
            if (options.recursive && targetInfo.isDir()) {
                return true;
            }
            FileSystemAccessInterfacePrivate::setFileSystemError(error, targetInfo.isDir() ? FileSystemErrorCode::AlreadyExists : FileSystemErrorCode::NotDirectory, QStringLiteral("mkdir"), nativePath(resolvedPath), targetInfo.isDir() ? tr("The directory already exists.") : tr("The requested path is not a directory."));
            return false;
        }

        const auto parentPath = Internal::normalizedFileSystemPath(targetInfo.absolutePath());
        if (!options.recursive) {
            if (!d->canonicalPermissionCheck(*execution, parentPath, FileAccessMode::Write, true, error, QStringLiteral("mkdir"))) {
                return false;
            }
            if (!QFileInfo(parentPath).isDir()) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotDirectory, QStringLiteral("mkdir"), nativePath(parentPath), tr("The parent directory does not exist."));
                return false;
            }
            if (!QDir(parentPath).mkdir(targetInfo.fileName())) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Io, QStringLiteral("mkdir"), nativePath(resolvedPath), tr("Failed to create the directory."));
                return false;
            }
            return true;
        }

        QString currentPath = parentPath;
        while (!currentPath.isEmpty()) {
            const QFileInfo currentInfo(currentPath);
            if (currentInfo.exists()) {
                if (!currentInfo.isDir()) {
                    FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotDirectory, QStringLiteral("mkdir"), nativePath(currentPath), tr("A parent path is not a directory."));
                    return false;
                }
                break;
            }
            if (!d->canonicalPermissionCheck(*execution, currentPath, FileAccessMode::Write, true, error, QStringLiteral("mkdir"))) {
                return false;
            }
            const auto nextPath = Internal::normalizedFileSystemPath(currentInfo.absolutePath());
            if (nextPath == currentPath) {
                break;
            }
            currentPath = nextPath;
        }
        if (!QDir().mkpath(resolvedPath)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::Io, QStringLiteral("mkdir"), nativePath(resolvedPath), tr("Failed to create the directory tree."));
            return false;
        }
        return true;
    }

    bool FileSystemAccessInterface::copy(ScriptExecutionContext *context, const QString &source, const QString &destination, const FileSystemCopyOptions &options, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("copy"), source, tr("File-system entries can only be copied while a script action is executing."), destination);
            return false;
        }
        QString sourcePath;
        QString destinationPath;
        if (!d->resolvePath(context, source, &sourcePath, error, QStringLiteral("copy")) || !d->resolvePath(context, destination, &destinationPath, error, QStringLiteral("copy"))) {
            return false;
        }

        if (!d->canonicalPermissionCheck(*execution, sourcePath, FileAccessMode::Read, false, error, QStringLiteral("copy"))) {
            return false;
        }
        const QFileInfo sourceInfo(sourcePath);
        const auto sourceIsLink = sourceInfo.isSymbolicLink() || sourceInfo.isJunction();
        if ((!sourceIsLink && !d->canonicalPermissionCheck(*execution, sourcePath, FileAccessMode::Read, true, error, QStringLiteral("copy"))) || !d->canonicalPermissionCheck(*execution, destinationPath, FileAccessMode::Write, true, error, QStringLiteral("copy"))) {
            return false;
        }
        if (!sourceInfo.exists() && !sourceIsLink) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("copy"), nativePath(sourcePath), tr("The source path does not exist."), nativePath(destinationPath));
            return false;
        }
        const auto sourceIsDirectory = sourceInfo.isDir() && !sourceIsLink;
        if (sourceIsDirectory && !options.recursive) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotSupported, QStringLiteral("copy"), nativePath(sourcePath), tr("Copying a directory requires recursive mode."), nativePath(destinationPath));
            return false;
        }
        if (sourceIsDirectory) {
            const auto canonicalSourcePath = canonicalOrDerivedPath(sourcePath);
            const auto canonicalDestinationPath = canonicalOrDerivedPath(destinationPath);
            if (!d->permissionCoversPattern(*execution, sourcePath + QStringLiteral("/**"), FileAccessMode::Read) || (!canonicalSourcePath.isEmpty() && !d->permissionCoversPattern(*execution, canonicalSourcePath + QStringLiteral("/**"), FileAccessMode::Read)) || !d->permissionCoversPattern(*execution, destinationPath + QStringLiteral("/**"), FileAccessMode::Write) || (!canonicalDestinationPath.isEmpty() && !d->permissionCoversPattern(*execution, canonicalDestinationPath + QStringLiteral("/**"), FileAccessMode::Write))) {
                FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(sourcePath), tr("Recursive copy requires permission for the complete source and destination directory trees."));
                return false;
            }
            if (Internal::isPathWithin(sourcePath, destinationPath)) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("copy"), nativePath(sourcePath), tr("A directory cannot be copied into itself."), nativePath(destinationPath));
                return false;
            }
        }

        const QFileInfo destinationInfo(destinationPath);
        const auto destinationExists = destinationInfo.exists() || destinationInfo.isSymbolicLink() || destinationInfo.isJunction();
        if (destinationExists && !options.overwrite) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::AlreadyExists, QStringLiteral("copy"), nativePath(sourcePath), tr("The destination path already exists."), nativePath(destinationPath));
            return false;
        }
        if (destinationExists && options.overwrite) {
            if (Internal::isPathWithin(destinationPath, sourcePath)) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("copy"), nativePath(sourcePath), tr("Replacing this destination would remove the source."), nativePath(destinationPath));
                return false;
            }
            if (destinationInfo.isDir() && !destinationInfo.isSymbolicLink() && !destinationInfo.isJunction()) {
                const auto canonicalDestinationPath = canonicalOrDerivedPath(destinationPath);
                if (!d->permissionCoversPattern(*execution, destinationPath + QStringLiteral("/**"), FileAccessMode::Write) || (!canonicalDestinationPath.isEmpty() && !d->permissionCoversPattern(*execution, canonicalDestinationPath + QStringLiteral("/**"), FileAccessMode::Write))) {
                    FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(destinationPath), tr("Replacing this directory requires write permission for its complete directory tree."));
                    return false;
                }
            }
            std::error_code removeError;
            std::filesystem::remove_all(nativeFileSystemPath(destinationPath), removeError);
            if (removeError) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, errorCodeForSystemError(removeError), QStringLiteral("copy"), nativePath(sourcePath), QString::fromLocal8Bit(removeError.message().c_str()), nativePath(destinationPath));
                return false;
            }
            execution->invalidatedPaths.insert(destinationPath, ++execution->mutationSerial);
        }

        std::error_code copyError;
        auto copyOptions = std::filesystem::copy_options::copy_symlinks;
        if (sourceIsDirectory) {
            copyOptions |= std::filesystem::copy_options::recursive;
        }
        std::filesystem::copy(nativeFileSystemPath(sourcePath), nativeFileSystemPath(destinationPath), copyOptions, copyError);
        if (copyError) {
            const auto mappedCode = errorCodeForSystemError(copyError);
            const auto code = sourceIsLink && (mappedCode == FileSystemErrorCode::Io || mappedCode == FileSystemErrorCode::ReadOnly) ? FileSystemErrorCode::NotSupported : mappedCode;
            FileSystemAccessInterfacePrivate::setFileSystemError(error, code, QStringLiteral("copy"), nativePath(sourcePath), QString::fromLocal8Bit(copyError.message().c_str()), nativePath(destinationPath));
            return false;
        }
        return true;
    }

    bool FileSystemAccessInterface::move(ScriptExecutionContext *context, const QString &source, const QString &destination, const FileSystemMoveOptions &options, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("move"), source, tr("File-system entries can only be moved while a script action is executing."), destination);
            return false;
        }
        QString sourcePath;
        QString destinationPath;
        if (!d->resolvePath(context, source, &sourcePath, error, QStringLiteral("move")) || !d->resolvePath(context, destination, &destinationPath, error, QStringLiteral("move"))) {
            return false;
        }
        if (!d->canonicalPermissionCheck(*execution, sourcePath, FileAccessMode::Write, false, error, QStringLiteral("move"))) {
            return false;
        }
        const QFileInfo sourceInfo(sourcePath);
        const auto sourceIsLink = sourceInfo.isSymbolicLink() || sourceInfo.isJunction();
        if ((!sourceIsLink && !d->canonicalPermissionCheck(*execution, sourcePath, FileAccessMode::Write, true, error, QStringLiteral("move"))) || !d->canonicalPermissionCheck(*execution, destinationPath, FileAccessMode::Write, true, error, QStringLiteral("move"))) {
            return false;
        }
        if (!sourceInfo.exists() && !sourceIsLink) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("move"), nativePath(sourcePath), tr("The source path does not exist."), nativePath(destinationPath));
            return false;
        }
        if (sourceInfo.isDir() && !sourceIsLink) {
            const auto canonicalSourcePath = canonicalOrDerivedPath(sourcePath);
            const auto canonicalDestinationPath = canonicalOrDerivedPath(destinationPath);
            if (!d->permissionCoversPattern(*execution, sourcePath + QStringLiteral("/**"), FileAccessMode::Write) || (!canonicalSourcePath.isEmpty() && !d->permissionCoversPattern(*execution, canonicalSourcePath + QStringLiteral("/**"), FileAccessMode::Write)) || !d->permissionCoversPattern(*execution, destinationPath + QStringLiteral("/**"), FileAccessMode::Write) || (!canonicalDestinationPath.isEmpty() && !d->permissionCoversPattern(*execution, canonicalDestinationPath + QStringLiteral("/**"), FileAccessMode::Write))) {
                FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(sourcePath), tr("Moving a directory requires write permission for the complete source and destination directory trees."));
                return false;
            }
            if (Internal::isPathWithin(sourcePath, destinationPath)) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("move"), nativePath(sourcePath), tr("A directory cannot be moved into itself."), nativePath(destinationPath));
                return false;
            }
        }

        const QFileInfo destinationInfo(destinationPath);
        const auto destinationExists = destinationInfo.exists() || destinationInfo.isSymbolicLink() || destinationInfo.isJunction();
        if (destinationExists && !options.overwrite) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::AlreadyExists, QStringLiteral("move"), nativePath(sourcePath), tr("The destination path already exists."), nativePath(destinationPath));
            return false;
        }
        if (destinationExists) {
            if (Internal::isPathWithin(destinationPath, sourcePath)) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("move"), nativePath(sourcePath), tr("Replacing this destination would remove the source."), nativePath(destinationPath));
                return false;
            }
            if (destinationInfo.isDir() && !destinationInfo.isSymbolicLink() && !destinationInfo.isJunction()) {
                const auto canonicalDestinationPath = canonicalOrDerivedPath(destinationPath);
                if (!d->permissionCoversPattern(*execution, destinationPath + QStringLiteral("/**"), FileAccessMode::Write) || (!canonicalDestinationPath.isEmpty() && !d->permissionCoversPattern(*execution, canonicalDestinationPath + QStringLiteral("/**"), FileAccessMode::Write))) {
                    FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(destinationPath), tr("Replacing this directory requires write permission for its complete directory tree."));
                    return false;
                }
            }
            std::error_code removeError;
            std::filesystem::remove_all(nativeFileSystemPath(destinationPath), removeError);
            if (removeError) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, errorCodeForSystemError(removeError), QStringLiteral("move"), nativePath(sourcePath), QString::fromLocal8Bit(removeError.message().c_str()), nativePath(destinationPath));
                return false;
            }
            execution->invalidatedPaths.insert(destinationPath, ++execution->mutationSerial);
        }

        std::error_code moveError;
        std::filesystem::rename(nativeFileSystemPath(sourcePath), nativeFileSystemPath(destinationPath), moveError);
        if (moveError) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, errorCodeForSystemError(moveError), QStringLiteral("move"), nativePath(sourcePath), QString::fromLocal8Bit(moveError.message().c_str()), nativePath(destinationPath));
            return false;
        }
        const auto mutationSerial = ++execution->mutationSerial;
        execution->invalidatedPaths.insert(sourcePath, mutationSerial);
        if (destinationExists) {
            execution->invalidatedPaths.insert(destinationPath, mutationSerial);
        }
        return true;
    }

    bool FileSystemAccessInterface::remove(ScriptExecutionContext *context, const QString &path, const FileSystemRemoveOptions &options, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("remove"), path, tr("File-system entries can only be removed while a script action is executing."));
            return false;
        }
        QString resolvedPath;
        if (!d->resolvePath(context, path, &resolvedPath, error, QStringLiteral("remove"))) {
            return false;
        }
        if (!d->canonicalPermissionCheck(*execution, resolvedPath, FileAccessMode::Write, false, error, QStringLiteral("remove"))) {
            return false;
        }
        const QFileInfo fileInfo(resolvedPath);
        const auto isLink = fileInfo.isSymbolicLink() || fileInfo.isJunction();
        if (!isLink && !d->canonicalPermissionCheck(*execution, resolvedPath, FileAccessMode::Write, true, error, QStringLiteral("remove"))) {
            return false;
        }
        if (!fileInfo.exists() && !isLink) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("remove"), nativePath(resolvedPath), tr("The requested path does not exist."));
            return false;
        }
        if (fileInfo.isDir() && !isLink && (options.useTrash || options.recursive)) {
            const auto canonicalPath = canonicalOrDerivedPath(resolvedPath);
            if (!d->permissionCoversPattern(*execution, resolvedPath + QStringLiteral("/**"), FileAccessMode::Write) || (!canonicalPath.isEmpty() && !d->permissionCoversPattern(*execution, canonicalPath + QStringLiteral("/**"), FileAccessMode::Write))) {
                FileSystemAccessInterfacePrivate::setPermissionError(error, nativePath(resolvedPath), tr("Removing this directory requires write permission for its complete directory tree."));
                return false;
            }
        }

        const auto mayPartiallyRemoveTree = fileInfo.isDir() && !isLink && !options.useTrash && options.recursive;
        if (mayPartiallyRemoveTree) {
            execution->invalidatedPaths.insert(resolvedPath, ++execution->mutationSerial);
        }
        if (options.useTrash) {
            if (!QFile::moveToTrash(resolvedPath)) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotSupported, QStringLiteral("remove"), nativePath(resolvedPath), tr("The operating system could not move this entry to the trash."));
                return false;
            }
        } else {
            std::error_code removeError;
            if (fileInfo.isDir() && !isLink && options.recursive) {
                std::filesystem::remove_all(nativeFileSystemPath(resolvedPath), removeError);
            } else {
                const auto removed = std::filesystem::remove(nativeFileSystemPath(resolvedPath), removeError);
                if (!removeError && !removed) {
                    removeError = std::make_error_code(std::errc::no_such_file_or_directory);
                }
            }
            if (removeError) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, errorCodeForSystemError(removeError), QStringLiteral("remove"), nativePath(resolvedPath), QString::fromLocal8Bit(removeError.message().c_str()));
                return false;
            }
        }
        if (!mayPartiallyRemoveTree) {
            execution->invalidatedPaths.insert(resolvedPath, ++execution->mutationSerial);
        }
        return true;
    }

    bool FileSystemAccessInterface::realPath(ScriptExecutionContext *context, const QString &path, QString *result, FileSystemError *error) const {
        Q_D(const FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        if (!result) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("realpath"), path, tr("No destination was provided for the resolved path."));
            return false;
        }
        const auto execution = d->executionForContext(context);
        if (!execution) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("realpath"), path, tr("Paths can only be resolved while a script action is executing."));
            return false;
        }
        QString resolvedPath;
        if (!d->resolvePath(context, path, &resolvedPath, error, QStringLiteral("realpath")) || !d->canonicalPermissionCheck(*execution, resolvedPath, FileAccessMode::Read, true, error, QStringLiteral("realpath"))) {
            return false;
        }
        const auto canonicalPath = Internal::normalizedFileSystemPath(QFileInfo(resolvedPath).canonicalFilePath());
        if (canonicalPath.isEmpty()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::NotFound, QStringLiteral("realpath"), nativePath(resolvedPath), tr("The requested path does not exist or could not be resolved."));
            return false;
        }
        *result = nativePath(canonicalPath);
        return true;
    }

    QString FileSystemAccessInterface::normalize(const QString &path) const {
        return QDir::toNativeSeparators(QDir::cleanPath(path));
    }

    bool FileSystemAccessInterface::resolve(ScriptExecutionContext *context, const QStringList &parts, QString *result, FileSystemError *error) const {
        if (error) {
            error->clear();
        }
        if (!result) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("resolve"), {}, tr("No destination was provided for the resolved path."));
            return false;
        }
        QString currentPath = context && context->script() ? context->script()->d_func()->rootPath : QString();
        for (const auto &part : parts) {
            if (part.contains(QChar::Null)) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("resolve"), part, tr("A path part contains an invalid null character."));
                return false;
            }
            if (part.isEmpty()) {
                continue;
            }
            currentPath = QDir::isAbsolutePath(part) ? part : QDir(currentPath).filePath(part);
        }
        if (!QDir::isAbsolutePath(currentPath)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("resolve"), currentPath, tr("The path cannot be resolved without an absolute path or a script root."));
            return false;
        }
        *result = nativePath(QFileInfo(QDir::cleanPath(currentPath)).absoluteFilePath());
        return true;
    }

    bool FileSystemAccessInterface::relative(ScriptExecutionContext *context, const QString &from, const QString &to, QString *result, FileSystemError *error) const {
        if (error) {
            error->clear();
        }
        if (!result) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("relative"), from, tr("No destination was provided for the relative path."), to);
            return false;
        }
        QString fromPath;
        QString toPath;
        if (!resolve(context, {from}, &fromPath, error) || !resolve(context, {to}, &toPath, error)) {
            return false;
        }
        const auto relativePath = QDir(fromPath).relativeFilePath(toPath);
        if (QDir::isAbsolutePath(relativePath)) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::CrossDevice, QStringLiteral("relative"), fromPath, tr("A relative path cannot be formed between these path roots."), toPath);
            return false;
        }
        *result = QDir::toNativeSeparators(relativePath == QStringLiteral(".") ? QString() : relativePath);
        return true;
    }

    bool FileSystemAccessInterface::isAbsolute(const QString &path) const {
        return QDir::isAbsolutePath(path);
    }

    QString FileSystemAccessInterface::join(const QStringList &parts) const {
        QString result;
        for (const auto &part : parts) {
            if (part.isEmpty()) {
                continue;
            }
            result = result.isEmpty() ? part : QDir(result).filePath(part);
        }
        return result.isEmpty() ? QString() : QDir::toNativeSeparators(QDir::cleanPath(result));
    }

    QString FileSystemAccessInterface::baseName(const QString &path) const {
        return QFileInfo(path).fileName();
    }

    QString FileSystemAccessInterface::directoryName(const QString &path) const {
        return QDir::toNativeSeparators(QFileInfo(path).path());
    }

    QString FileSystemAccessInterface::extension(const QString &path) const {
        return lastExtension(path);
    }

    FileSystemParsedPath FileSystemAccessInterface::parse(const QString &path) const {
        if (path.isEmpty()) {
            return {};
        }
        const auto normalizedPath = QDir::toNativeSeparators(QDir::cleanPath(path));
        const QFileInfo fileInfo(normalizedPath);
        FileSystemParsedPath result;
        result.directory = QDir::toNativeSeparators(fileInfo.path());
        if (result.directory == QStringLiteral(".") && !normalizedPath.contains(QLatin1Char('/')) && !normalizedPath.contains(QLatin1Char('\\'))) {
            result.directory.clear();
        }
        result.baseName = fileInfo.fileName();
        result.extension = lastExtension(normalizedPath);
        result.name = result.extension.isEmpty() ? result.baseName : result.baseName.left(result.baseName.size() - result.extension.size());
        if (QDir::isAbsolutePath(normalizedPath)) {
            result.root = pathRoot(normalizedPath);
        }
        return result;
    }

    bool FileSystemAccessInterface::format(const FileSystemParsedPath &path, QString *result, FileSystemError *error) const {
        if (error) {
            error->clear();
        }
        const auto invalidBaseName = path.baseName.contains(QLatin1Char('/')) || path.baseName.contains(QLatin1Char('\\'));
        const auto invalidExtension = !path.extension.isEmpty() && !path.extension.startsWith(QLatin1Char('.'));
        const auto inconsistentAbsoluteDirectory = !path.root.isEmpty() && QDir::isAbsolutePath(path.directory) && !Internal::isPathWithin(path.root, path.directory);
        if (!result || invalidBaseName || invalidExtension || inconsistentAbsoluteDirectory || path.baseName != path.name + path.extension || (!path.root.isEmpty() && !QDir::isAbsolutePath(path.root))) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("format"), path.directory, tr("The path components are inconsistent."));
            return false;
        }
        auto directory = path.directory;
        if (directory.isEmpty()) {
            directory = path.root;
        } else if (!path.root.isEmpty() && !QDir::isAbsolutePath(directory)) {
            directory = QDir(path.root).filePath(directory);
        }
        *result = directory.isEmpty() ? path.baseName : QDir::toNativeSeparators(QDir(directory).filePath(path.baseName));
        return true;
    }

    bool FileSystemAccessInterface::matchesPattern(ScriptExecutionContext *context, const QString &path, const QString &pattern, bool *result, FileSystemError *error) const {
        Q_D(const FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        if (!result) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("match"), path, tr("No destination was provided for the pattern result."));
            return false;
        }
        QString resolvedPath;
        QString normalizedPattern;
        if (!d->resolvePath(context, path, &resolvedPath, error, QStringLiteral("match")) || !d->normalizePattern(context, pattern, &normalizedPattern, error)) {
            return false;
        }
        QString errorMessage;
        *result = Internal::matchesFileSystemPattern(resolvedPath, normalizedPattern, &errorMessage);
        if (!errorMessage.isEmpty()) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidPath, QStringLiteral("match"), pattern, errorMessage);
            return false;
        }
        return true;
    }

    QJSValue FileSystemAccessInterface::toJavaScriptValue(ScriptExecutionContext *context, const FileSystemHandle &handle, FileSystemError *error) {
        Q_D(FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        if (!validateHandle(context, handle, handle.access(), handle.kind(), error) || !d->moduleExtension) {
            if (!error || !error->hasError()) {
                FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), handle.path(), tr("The JavaScript file-system module is not available."));
            }
            return QJSValue(QJSValue::UndefinedValue);
        }
        return d->moduleExtension->wrapHandle(context, handle, error);
    }

    bool FileSystemAccessInterface::fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, FileSystemHandle *handle, FileSystemHandle::Kind expectedKind, FileSystemError *error) const {
        Q_D(const FileSystemAccessInterface);
        if (error) {
            error->clear();
        }
        if (!handle || !d->moduleExtension) {
            FileSystemAccessInterfacePrivate::setFileSystemError(error, FileSystemErrorCode::InvalidState, QStringLiteral("handle"), {}, tr("The JavaScript file-system module is not available."));
            return false;
        }
        return d->moduleExtension->unwrapHandle(context, value, handle, expectedKind, error);
    }

}

#include "moc_FileSystemAccessInterface.cpp"
