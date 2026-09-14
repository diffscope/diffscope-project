// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_FILESYSTEMHANDLE_P_H
#define DIFFSCOPE_BATCHPROCESS_FILESYSTEMHANDLE_P_H

#include <batchprocess/FileSystemHandle.h>

#include <QJSEngine>
#include <QPointer>
#include <QSharedData>
#include <QSharedPointer>

namespace BatchProcess {

    class ScriptAction;
    class ScriptExecutionContext;

    namespace Internal {

        struct FileSystemExecutionToken {
            bool active{};
            quint64 serial{};
            QPointer<QJSEngine> engine;
            QPointer<ScriptExecutionContext> context;
            QPointer<ScriptAction> action;
        };

    }

    class FileSystemHandlePrivate : public QSharedData {
    public:
        FileSystemHandle::Kind kind{FileSystemHandle::Invalid};
        QString path;
        FileAccessMode access{FileAccessMode::Read};
        bool recursive{};
        bool intrinsicCapability{};
        QString scopePath;
        bool scopeRecursive{};
        quint64 createdAtMutationSerial{};
        QSharedPointer<Internal::FileSystemExecutionToken> token;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_FILESYSTEMHANDLE_P_H
