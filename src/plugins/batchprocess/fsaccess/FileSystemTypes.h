// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_FILESYSTEMTYPES_H
#define DIFFSCOPE_BATCHPROCESS_FILESYSTEMTYPES_H

#include <optional>

#include <QDateTime>
#include <QString>
#include <QStringList>

#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    enum class FileAccessMode {
        Read,
        Write,
        ReadWrite,
    };

    enum class FileSystemEntryKind {
        File,
        Directory,
        SymbolicLink,
        Other,
    };

    enum class FileWriteDisposition {
        Replace,
        CreateNew,
        Append,
    };

    enum class FileSystemErrorKind {
        None,
        PermissionDenied,
        FileSystem,
    };

    enum class FileSystemErrorCode {
        None,
        NotFound,
        AlreadyExists,
        NotFile,
        NotDirectory,
        NotEmpty,
        InvalidPath,
        InvalidState,
        ReadOnly,
        Busy,
        OutOfSpace,
        CrossDevice,
        NotSupported,
        TooLarge,
        Io,
        Encoding,
        Unknown,
    };

    enum class FileSystemPermissionDecision {
        Allowed,
        Denied,
        Failed,
    };

    struct BATCH_PROCESS_EXPORT FileSystemError {
        FileSystemErrorKind kind{FileSystemErrorKind::None};
        FileSystemErrorCode code{FileSystemErrorCode::None};
        QString message;
        QString operation;
        QString path;
        QString destinationPath;

        bool hasError() const;
        void clear();
    };

    struct BATCH_PROCESS_EXPORT FileSystemStat {
        QString path;
        QString canonicalPath;
        bool exists{};
        std::optional<FileSystemEntryKind> kind;
        std::optional<FileSystemEntryKind> resolvedKind;
        std::optional<qint64> size;
        QDateTime createdAt;
        QDateTime modifiedAt;
        QDateTime accessedAt;
        QDateTime metadataChangedAt;
        QString symbolicLinkTarget;
        bool hidden{};
        std::optional<bool> readable;
        std::optional<bool> writable;
        std::optional<bool> executable;
    };

    struct BATCH_PROCESS_EXPORT FileSystemDirectoryEntry {
        QString relativePath;
        QString path;
        FileSystemEntryKind kind{FileSystemEntryKind::Other};
        std::optional<FileSystemEntryKind> resolvedKind;
    };

    struct BATCH_PROCESS_EXPORT FileSystemPermissionRequest {
        QString pathPattern;
        FileAccessMode access{FileAccessMode::Read};
        QString reason;
    };

    struct BATCH_PROCESS_EXPORT FileSystemStatOptions {
        bool followSymbolicLinks{true};
    };

    struct BATCH_PROCESS_EXPORT FileTextReadOptions {
        QString encoding{QStringLiteral("utf-8")};
        std::optional<qint64> maximumBytes;
    };

    struct BATCH_PROCESS_EXPORT FileBinaryReadOptions {
        std::optional<qint64> maximumBytes;
    };

    struct BATCH_PROCESS_EXPORT FileTextWriteOptions {
        QString encoding{QStringLiteral("utf-8")};
        FileWriteDisposition disposition{FileWriteDisposition::Replace};
        std::optional<bool> atomic;
        bool createParents{};
    };

    struct BATCH_PROCESS_EXPORT FileBinaryWriteOptions {
        FileWriteDisposition disposition{FileWriteDisposition::Replace};
        std::optional<bool> atomic;
        bool createParents{};
    };

    struct BATCH_PROCESS_EXPORT FileSystemDirectoryListOptions {
        bool recursive{};
        bool files{true};
        bool directories{true};
        QStringList nameFilters;
    };

    struct BATCH_PROCESS_EXPORT FileSystemCreateDirectoryOptions {
        bool recursive{};
    };

    struct BATCH_PROCESS_EXPORT FileSystemCopyOptions {
        bool overwrite{};
        bool recursive{};
    };

    struct BATCH_PROCESS_EXPORT FileSystemMoveOptions {
        bool overwrite{};
    };

    struct BATCH_PROCESS_EXPORT FileSystemRemoveOptions {
        bool recursive{};
        bool useTrash{true};
    };

    struct BATCH_PROCESS_EXPORT FileSystemParsedPath {
        QString root;
        QString directory;
        QString baseName;
        QString name;
        QString extension;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_FILESYSTEMTYPES_H
