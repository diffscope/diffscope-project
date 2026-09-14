// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_FILESYSTEMHANDLE_H
#define DIFFSCOPE_BATCHPROCESS_FILESYSTEMHANDLE_H

#include <QExplicitlySharedDataPointer>
#include <QString>

#include <batchprocess/FileSystemTypes.h>
#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    namespace Internal {
        class FileSystemHandleBridge;
    }

    class FileSystemAccessInterface;
    class FileSystemAccessInterfacePrivate;
    class FileSystemHandlePrivate;

    class BATCH_PROCESS_EXPORT FileSystemHandle {
    public:
        enum Kind {
            Invalid,
            File,
            Directory,
        };

        FileSystemHandle();
        FileSystemHandle(const FileSystemHandle &other);
        FileSystemHandle(FileSystemHandle &&other) noexcept;
        ~FileSystemHandle();

        FileSystemHandle &operator=(const FileSystemHandle &other);
        FileSystemHandle &operator=(FileSystemHandle &&other) noexcept;

        bool isValid() const;
        Kind kind() const;
        QString path() const;
        FileAccessMode access() const;
        bool recursive() const;

    private:
        friend class FileSystemAccessInterface;
        friend class FileSystemAccessInterfacePrivate;
        friend class Internal::FileSystemHandleBridge;

        explicit FileSystemHandle(FileSystemHandlePrivate *d);

        QExplicitlySharedDataPointer<FileSystemHandlePrivate> d;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_FILESYSTEMHANDLE_H
