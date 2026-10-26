// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "FileOpenDispatcher.h"
#include "FileOpenDispatcher_p.h"

#include <utility>

#include <QDir>
#include <QLoggingCategory>
#include <QQmlEngine>
#include <QScopedValueRollback>
#include <QThread>

#include <coreplugin/private/FileOpenPlan_p.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcFileOpenDispatcher, "diffscope.core.fileopendispatcher")

    namespace {

        QStringList absoluteFilePaths(const QStringList &filePaths) {
            const QDir directory;
            QStringList paths;
            for (const auto &path : filePaths) {
                if (!path.isEmpty()) {
                    paths.append(QDir::cleanPath(directory.absoluteFilePath(path)));
                }
            }
            return paths;
        }

        QSet<qsizetype> checkedIndexes(const QSet<qsizetype> &indexes, qsizetype size) {
            QSet<qsizetype> result;
            for (const auto index : indexes) {
                if (index < 0 || index >= size) {
                    qCWarning(lcFileOpenDispatcher) << "File handler returned an invalid index:" << index;
                    continue;
                }
                result.insert(index);
            }
            return result;
        }

    }

    bool FileOpenRequest::isValid() const {
        if (hadParentWindow && !parentWindow) {
            return false;
        }
        if (planned) {
            for (const auto &registration : handlers) {
                if (!registration->active || !registration->handler) {
                    return false;
                }
            }
        }
        return true;
    }

    void FileOpenDispatcherPrivate::unregisterRegistration(QSharedPointer<FileOpenHandlerRegistration> registration) {
        if (!registration->active) {
            return;
        }
        registration->active = false;
        QObject::disconnect(registration->destructionConnection);
        handlers.removeOne(registration);
        const auto currentPlans = plans;
        for (const auto &plan : currentPlans) {
            if (plan && !plan->isValid()) {
                plan->invalidate();
            }
        }
    }

    void FileOpenDispatcherPrivate::processRequests() {
        if (!ready || processing) {
            return;
        }
        QScopedValueRollback guard(processing, true);
        while (ready && !pendingRequests.isEmpty()) {
            auto request = pendingRequests.dequeue();
            if (!request->planned) {
                request->handlers = handlers;
            }
            if (request->isValid()) {
                dispatch(request);
            }
        }
    }

    void FileOpenDispatcherPrivate::dispatch(const QSharedPointer<FileOpenRequest> &request) {
        QList<qsizetype> remaining;
        for (qsizetype index = 0; index < request->filePaths.size(); ++index) {
            remaining.append(index);
        }
        for (qsizetype handlerIndex = 0; handlerIndex < request->handlers.size() && !remaining.isEmpty(); ++handlerIndex) {
            const auto registration = request->handlers.at(handlerIndex);
            if (!registration->active || !registration->handler) {
                continue;
            }
            QStringList batch;
            QList<qsizetype> originalIndexes;
            for (const auto index : remaining) {
                if (!request->planned || request->owners.at(index) < 0 || request->owners.at(index) == handlerIndex) {
                    batch.append(request->filePaths.at(index));
                    originalIndexes.append(index);
                }
            }
            if (batch.isEmpty()) {
                continue;
            }
            const auto claimed = checkedIndexes(registration->handler->openFiles(batch, request->parentWindow), batch.size());
            QSet<qsizetype> consumed;
            for (qsizetype index = 0; index < originalIndexes.size(); ++index) {
                const auto originalIndex = originalIndexes.at(index);
                if (claimed.contains(index) || (request->planned && request->owners.at(originalIndex) == handlerIndex)) {
                    consumed.insert(originalIndex);
                }
            }
            remaining.removeIf([&consumed](qsizetype index) { return consumed.contains(index); });
        }
    }

    bool FileOpenDispatcherPrivate::submit(const QSharedPointer<FileOpenRequest> &request) {
        Q_Q(FileOpenDispatcher);
        if (!request->isValid()) {
            return false;
        }
        pendingRequests.enqueue(request);
        QMetaObject::invokeMethod(q, [this] { processRequests(); }, Qt::QueuedConnection);
        return true;
    }

    FileOpenDispatcher::FileOpenDispatcher(QObject *parent) : QObject(parent), d_ptr(new FileOpenDispatcherPrivate) {
        Q_D(FileOpenDispatcher);
        d->q_ptr = this;
    }

    FileOpenDispatcher::~FileOpenDispatcher() = default;

    bool FileOpenDispatcher::registerHandler(IFileOpenHandler *handler) {
        Q_D(FileOpenDispatcher);
        Q_ASSERT(QThread::currentThread() == thread());
        if (!handler || handler->thread() != thread()) {
            return false;
        }
        for (const auto &registration : d->handlers) {
            if (registration->handler == handler) {
                return false;
            }
        }
        auto registration = QSharedPointer<FileOpenHandlerRegistration>::create();
        registration->handler = handler;
        registration->destructionConnection = connect(handler, &QObject::destroyed, this, [this, registration] {
            Q_D(FileOpenDispatcher);
            d->unregisterRegistration(registration);
        });
        d->handlers.append(registration);
        return true;
    }

    bool FileOpenDispatcher::unregisterHandler(IFileOpenHandler *handler) {
        Q_D(FileOpenDispatcher);
        Q_ASSERT(QThread::currentThread() == thread());
        if (!handler) {
            return false;
        }
        for (const auto &registration : std::as_const(d->handlers)) {
            if (registration->handler == handler) {
                const auto removedRegistration = registration;
                d->unregisterRegistration(removedRegistration);
                return true;
            }
        }
        return false;
    }

    void FileOpenDispatcher::openFiles(const QStringList &filePaths, QWindow *parentWindow) {
        Q_D(FileOpenDispatcher);
        Q_ASSERT(QThread::currentThread() == thread());
        auto request = QSharedPointer<FileOpenRequest>::create();
        request->filePaths = absoluteFilePaths(filePaths);
        if (request->filePaths.isEmpty()) {
            return;
        }
        request->parentWindow = parentWindow;
        request->hadParentWindow = parentWindow != nullptr;
        d->pendingRequests.enqueue(request);
        d->processRequests();
    }

    FileOpenPlan *FileOpenDispatcher::prepareDrop(const QList<QUrl> &urls, QWindow *parentWindow) {
        Q_D(FileOpenDispatcher);
        Q_ASSERT(QThread::currentThread() == thread());
        auto request = QSharedPointer<FileOpenRequest>::create();
        QStringList paths;
        for (const auto &url : urls) {
            if (url.isValid() && url.isLocalFile()) {
                paths.append(url.toLocalFile());
            }
        }
        request->filePaths = absoluteFilePaths(paths);
        request->parentWindow = parentWindow;
        request->hadParentWindow = parentWindow != nullptr;
        request->planned = true;
        request->handlers = d->handlers;
        for (qsizetype index = 0; index < request->filePaths.size(); ++index) {
            request->owners.append(-1);
        }
        bool accepted = false;
        if (d->ready) {
            for (qsizetype handlerIndex = 0; handlerIndex < request->handlers.size(); ++handlerIndex) {
                if (!request->isValid()) {
                    break;
                }
                QStringList batch;
                QList<qsizetype> originalIndexes;
                for (qsizetype index = 0; index < request->filePaths.size(); ++index) {
                    if (request->owners.at(index) < 0) {
                        batch.append(request->filePaths.at(index));
                        originalIndexes.append(index);
                    }
                }
                if (batch.isEmpty()) {
                    break;
                }
                const auto &registration = request->handlers.at(handlerIndex);
                const auto indexes = checkedIndexes(registration->handler->preflight(batch, request->parentWindow), batch.size());
                for (const auto index : indexes) {
                    request->owners[originalIndexes.at(index)] = handlerIndex;
                    accepted = true;
                }
            }
        }
        auto planData = new FileOpenPlanPrivate;
        planData->dispatcher = this;
        planData->request = request;
        planData->acceptsDrop = accepted;
        auto plan = new FileOpenPlan(*planData, this);
        QQmlEngine::setObjectOwnership(plan, QQmlEngine::CppOwnership);
        if (parentWindow) {
            connect(parentWindow, &QObject::destroyed, plan, &FileOpenPlan::invalidate);
        }
        d->plans.append(plan);
        connect(plan, &QObject::destroyed, this, [this] {
            Q_D(FileOpenDispatcher);
            d->plans.removeIf([](const auto &item) { return item.isNull(); });
        });
        return plan;
    }

    void FileOpenDispatcher::setReady(bool ready) {
        Q_D(FileOpenDispatcher);
        d->ready = ready;
        d->processRequests();
    }

}

#include "moc_FileOpenDispatcher.cpp"
