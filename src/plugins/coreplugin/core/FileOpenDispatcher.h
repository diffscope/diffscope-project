// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_FILEOPENDISPATCHER_H
#define DIFFSCOPE_COREPLUGIN_FILEOPENDISPATCHER_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <QUrl>
#include <qqmlintegration.h>

#include <coreplugin/FileOpenPlan.h>
#include <coreplugin/coreglobal.h>

class QWindow;

namespace Core {

    namespace Internal {
        class CorePlugin;
    }

    class IFileOpenHandler;
    class FileOpenDispatcherPrivate;

    class CORE_EXPORT FileOpenDispatcher : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(FileOpenDispatcher)
    public:
        explicit FileOpenDispatcher(QObject *parent = nullptr);
        ~FileOpenDispatcher() override;

        // Appends a handler without transferring ownership.
        // Returns false if the handler is null, already registered, or owned by another thread.
        bool registerHandler(IFileOpenHandler *handler);
        // Removes a handler and invalidates its pending plans. Returns false if absent.
        bool unregisterHandler(IFileOpenHandler *handler);

        // Dispatches nonempty paths in order. Relative paths use the current working directory.
        Q_INVOKABLE void openFiles(const QStringList &filePaths, QWindow *parentWindow = nullptr);
        // Returns a plan for local file URLs, parented to this dispatcher. The caller may delete the plan.
        Q_INVOKABLE FileOpenPlan *prepareDrop(const QList<QUrl> &urls, QWindow *parentWindow = nullptr);

    private:
        void setReady(bool ready);

        QScopedPointer<FileOpenDispatcherPrivate> d_ptr;

        friend class FileOpenPlan;
        friend class Internal::CorePlugin;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_FILEOPENDISPATCHER_H
