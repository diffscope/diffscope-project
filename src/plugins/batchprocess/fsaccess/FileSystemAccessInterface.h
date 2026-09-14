// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_FILESYSTEMACCESSINTERFACE_H
#define DIFFSCOPE_BATCHPROCESS_FILESYSTEMACCESSINTERFACE_H

#include <QByteArray>
#include <QByteArrayView>
#include <QJSValue>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QStringList>

#include <batchprocess/FileSystemHandle.h>
#include <batchprocess/FileSystemTypes.h>
#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    namespace Internal {
        class BatchProcessPlugin;
        class FileSystemModuleBridge;
        class FileSystemModuleExtension;
    }

    class FileSystemAccessInterfacePrivate;
    class ScriptExecutionContext;

    class BATCH_PROCESS_EXPORT FileSystemAccessInterface : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(FileSystemAccessInterface)
    public:
        ~FileSystemAccessInterface() override;

        static FileSystemAccessInterface *instance();

        FileSystemPermissionDecision requestPermission(ScriptExecutionContext *context, const FileSystemPermissionRequest &request, FileSystemError *error = nullptr);
        bool hasPermission(ScriptExecutionContext *context, const QString &path, FileAccessMode access, bool recursive = false) const;

        FileSystemHandle getFile(ScriptExecutionContext *context, const QString &path, FileAccessMode access = FileAccessMode::Read, FileSystemError *error = nullptr);
        FileSystemHandle getDirectory(ScriptExecutionContext *context, const QString &path, FileAccessMode access = FileAccessMode::Read, bool recursive = false, FileSystemError *error = nullptr);
        FileSystemHandle scriptPackage(ScriptExecutionContext *context, FileSystemError *error = nullptr);
        FileSystemHandle dataDirectory(ScriptExecutionContext *context, FileSystemError *error = nullptr);
        FileSystemHandle temporaryDirectory(ScriptExecutionContext *context, FileSystemError *error = nullptr);

        /** Creates a capability after a trusted host component has independently obtained user authorization. */
        FileSystemHandle createAuthorizedFileHandle(ScriptExecutionContext *context, const QString &path, FileAccessMode access, FileSystemError *error = nullptr);
        /** Creates a capability after a trusted host component has independently obtained user authorization. */
        FileSystemHandle createAuthorizedDirectoryHandle(ScriptExecutionContext *context, const QString &path, FileAccessMode access, bool recursive, FileSystemError *error = nullptr);
        bool validateHandle(ScriptExecutionContext *context, const FileSystemHandle &handle, FileAccessMode access, FileSystemHandle::Kind expectedKind = FileSystemHandle::Invalid, FileSystemError *error = nullptr) const;

        bool stat(ScriptExecutionContext *context, const QString &path, const FileSystemStatOptions &options, FileSystemStat *result, FileSystemError *error = nullptr) const;
        bool stat(ScriptExecutionContext *context, const FileSystemHandle &handle, const FileSystemStatOptions &options, FileSystemStat *result, FileSystemError *error = nullptr) const;
        bool readText(ScriptExecutionContext *context, const FileSystemHandle &handle, const FileTextReadOptions &options, QString *result, FileSystemError *error = nullptr) const;
        bool readBytes(ScriptExecutionContext *context, const FileSystemHandle &handle, const FileBinaryReadOptions &options, QByteArray *result, FileSystemError *error = nullptr) const;
        bool writeText(ScriptExecutionContext *context, const FileSystemHandle &handle, const QString &text, const FileTextWriteOptions &options, FileSystemError *error = nullptr);
        bool writeBytes(ScriptExecutionContext *context, const FileSystemHandle &handle, QByteArrayView data, const FileBinaryWriteOptions &options, FileSystemError *error = nullptr);
        bool entries(ScriptExecutionContext *context, const FileSystemHandle &handle, const FileSystemDirectoryListOptions &options, QList<FileSystemDirectoryEntry> *result, FileSystemError *error = nullptr) const;
        FileSystemHandle childFile(ScriptExecutionContext *context, const FileSystemHandle &directory, const QString &relativePath, FileAccessMode access = FileAccessMode::Read, FileSystemError *error = nullptr);
        FileSystemHandle childDirectory(ScriptExecutionContext *context, const FileSystemHandle &directory, const QString &relativePath, FileSystemError *error = nullptr);

        bool createDirectory(ScriptExecutionContext *context, const QString &path, const FileSystemCreateDirectoryOptions &options, FileSystemError *error = nullptr);
        bool copy(ScriptExecutionContext *context, const QString &source, const QString &destination, const FileSystemCopyOptions &options, FileSystemError *error = nullptr);
        bool move(ScriptExecutionContext *context, const QString &source, const QString &destination, const FileSystemMoveOptions &options, FileSystemError *error = nullptr);
        bool remove(ScriptExecutionContext *context, const QString &path, const FileSystemRemoveOptions &options, FileSystemError *error = nullptr);
        bool realPath(ScriptExecutionContext *context, const QString &path, QString *result, FileSystemError *error = nullptr) const;

        QString normalize(const QString &path) const;
        bool resolve(ScriptExecutionContext *context, const QStringList &parts, QString *result, FileSystemError *error = nullptr) const;
        bool relative(ScriptExecutionContext *context, const QString &from, const QString &to, QString *result, FileSystemError *error = nullptr) const;
        bool isAbsolute(const QString &path) const;
        QString join(const QStringList &parts) const;
        QString baseName(const QString &path) const;
        QString directoryName(const QString &path) const;
        QString extension(const QString &path) const;
        FileSystemParsedPath parse(const QString &path) const;
        bool format(const FileSystemParsedPath &path, QString *result, FileSystemError *error = nullptr) const;
        bool matchesPattern(ScriptExecutionContext *context, const QString &path, const QString &pattern, bool *result, FileSystemError *error = nullptr) const;

        QJSValue toJavaScriptValue(ScriptExecutionContext *context, const FileSystemHandle &handle, FileSystemError *error = nullptr);
        bool fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, FileSystemHandle *handle, FileSystemHandle::Kind expectedKind = FileSystemHandle::Invalid, FileSystemError *error = nullptr) const;

    private:
        friend class Internal::BatchProcessPlugin;
        friend class Internal::FileSystemModuleBridge;
        friend class Internal::FileSystemModuleExtension;

        explicit FileSystemAccessInterface(QObject *parent = nullptr);

        QScopedPointer<FileSystemAccessInterfacePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_FILESYSTEMACCESSINTERFACE_H
