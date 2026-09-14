// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_FILESYSTEMMODULEEXTENSION_H
#define DIFFSCOPE_BATCHPROCESS_FILESYSTEMMODULEEXTENSION_H

#include <QHash>
#include <QJSValue>
#include <QObject>
#include <QString>

#include <batchprocess/FileSystemHandle.h>

class QJSEngine;

namespace BatchProcess {

    class BatchProcessInterface;
    class FileSystemAccessInterface;
    struct FileSystemError;
    class ScriptExecutionContext;

}

namespace BatchProcess::Internal {

    class FileSystemModuleBridge;

    class FileSystemModuleExtension : public QObject {
        Q_OBJECT
    public:
        explicit FileSystemModuleExtension(BatchProcessInterface *batchProcessInterface, FileSystemAccessInterface *fileSystemAccessInterface, QObject *parent = nullptr);
        ~FileSystemModuleExtension() override;

        QJSValue wrapHandle(ScriptExecutionContext *context, const FileSystemHandle &handle, FileSystemError *error) const;
        bool unwrapHandle(ScriptExecutionContext *context, const QJSValue &value, FileSystemHandle *handle, FileSystemHandle::Kind expectedKind, FileSystemError *error) const;

    private:
        friend class FileSystemModuleBridge;

        void installIntoEngine(QJSEngine *engine, ScriptExecutionContext *context);
        QJSValue createModule(ScriptExecutionContext *context) const;
        QJSValue helper(ScriptExecutionContext *context, const QString &name, FileSystemError *error) const;

        BatchProcessInterface *m_batchProcessInterface;
        FileSystemAccessInterface *m_fileSystemAccessInterface;
        QHash<QJSEngine *, QJSValue> m_engineBundles;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_FILESYSTEMMODULEEXTENSION_H
