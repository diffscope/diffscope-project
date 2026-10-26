// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_FILEOPENDISPATCHER_P_H
#define DIFFSCOPE_COREPLUGIN_FILEOPENDISPATCHER_P_H

#include <coreplugin/FileOpenDispatcher.h>

#include <QPointer>
#include <QQueue>
#include <QSharedPointer>
#include <QWindow>

#include <coreplugin/IFileOpenHandler.h>

namespace Core {

    class FileOpenHandlerRegistration {
    public:
        QPointer<IFileOpenHandler> handler;
        QMetaObject::Connection destructionConnection;
        bool active = true;
    };

    class FileOpenRequest {
    public:
        QStringList filePaths;
        QPointer<QWindow> parentWindow;
        bool hadParentWindow = false;
        bool planned = false;
        QList<QSharedPointer<FileOpenHandlerRegistration>> handlers;
        QList<qsizetype> owners;

        bool isValid() const;
    };

    class FileOpenDispatcherPrivate {
        Q_DECLARE_PUBLIC(FileOpenDispatcher)
    public:
        FileOpenDispatcher *q_ptr = nullptr;
        QList<QSharedPointer<FileOpenHandlerRegistration>> handlers;
        QList<QPointer<FileOpenPlan>> plans;
        QQueue<QSharedPointer<FileOpenRequest>> pendingRequests;
        bool ready = true;
        bool processing = false;

        void unregisterRegistration(QSharedPointer<FileOpenHandlerRegistration> registration);
        void processRequests();
        void dispatch(const QSharedPointer<FileOpenRequest> &request);
        bool submit(const QSharedPointer<FileOpenRequest> &request);
    };

}

#endif // DIFFSCOPE_COREPLUGIN_FILEOPENDISPATCHER_P_H
