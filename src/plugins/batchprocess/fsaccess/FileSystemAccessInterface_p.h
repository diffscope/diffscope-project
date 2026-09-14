// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_FILESYSTEMACCESSINTERFACE_P_H
#define DIFFSCOPE_BATCHPROCESS_FILESYSTEMACCESSINTERFACE_P_H

#include <batchprocess/FileSystemAccessInterface.h>

#include <memory>

#include <QHash>
#include <QPointer>
#include <QSharedPointer>
#include <QTemporaryDir>
#include <QVector>

#include <batchprocess/private/FileSystemHandle_p.h>

class QJSEngine;

namespace BatchProcess {

    namespace Internal {
        class FileSystemModuleExtension;
    }

    class FileSystemAccessInterfacePrivate {
        Q_DECLARE_PUBLIC(FileSystemAccessInterface)
    public:
        struct PermissionGrant {
            QString pattern;
            FileAccessMode access{FileAccessMode::Read};
        };

        struct SessionPermissions {
            QVector<PermissionGrant> grants;
            bool fullAccess{};
        };

        struct ExecutionState {
            QSharedPointer<Internal::FileSystemExecutionToken> token;
            QString scriptId;
            QString scriptRoot;
            QVector<PermissionGrant> grants;
            std::unique_ptr<QTemporaryDir> temporaryDirectory;
            QString temporaryDirectoryPath;
            QHash<QString, quint64> invalidatedPaths;
            quint64 mutationSerial{};
            bool cleanupScheduled{};
        };

        explicit FileSystemAccessInterfacePrivate(FileSystemAccessInterface *q);
        ~FileSystemAccessInterfacePrivate();

        void beginExecution(ScriptExecutionContext *context);
        void scheduleEndExecution(ScriptExecutionContext *context);
        void finalizeExecution(quint64 serial);
        std::shared_ptr<ExecutionState> executionForContext(ScriptExecutionContext *context) const;

        bool resolvePath(ScriptExecutionContext *context, const QString &path, QString *result, FileSystemError *error, const QString &operation) const;
        bool normalizePattern(ScriptExecutionContext *context, const QString &pattern, QString *result, FileSystemError *error) const;
        bool permissionCoversPath(const ExecutionState &execution, const QString &path, FileAccessMode access) const;
        bool permissionCoversPattern(const ExecutionState &execution, const QString &pattern, FileAccessMode access) const;
        bool canonicalPermissionCheck(const ExecutionState &execution, const QString &path, FileAccessMode access, bool followTarget, FileSystemError *error, const QString &operation) const;
        bool handleCoversPath(const FileSystemHandle &handle, const QString &path) const;

        FileSystemHandle createHandle(const std::shared_ptr<ExecutionState> &execution, FileSystemHandle::Kind kind, const QString &path, FileAccessMode access, bool recursive, bool intrinsicCapability, const QString &scopePath = {}, bool scopeRecursive = false) const;

        static bool accessCovers(FileAccessMode available, FileAccessMode requested);
        static void setPermissionError(FileSystemError *error, const QString &path, const QString &message);
        static void setFileSystemError(FileSystemError *error, FileSystemErrorCode code, const QString &operation, const QString &path, const QString &message, const QString &destinationPath = {});

        FileSystemAccessInterface *q_ptr;
        QHash<QString, SessionPermissions> sessionPermissions;
        std::shared_ptr<ExecutionState> currentExecution;
        quint64 nextExecutionSerial{};
        QPointer<Internal::FileSystemModuleExtension> moduleExtension;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_FILESYSTEMACCESSINTERFACE_P_H
