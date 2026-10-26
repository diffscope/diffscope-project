// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "FileOpenPlan.h"
#include "FileOpenPlan_p.h"

#include <QThread>

#include <coreplugin/private/FileOpenDispatcher_p.h>

namespace Core {

    FileOpenPlan::FileOpenPlan(FileOpenPlanPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;
    }

    FileOpenPlan::~FileOpenPlan() = default;

    bool FileOpenPlan::acceptsDrop() const {
        Q_D(const FileOpenPlan);
        return d->acceptsDrop;
    }

    bool FileOpenPlan::isValid() const {
        Q_D(const FileOpenPlan);
        return !d->invalidated && !d->submitted && d->dispatcher && d->request->isValid();
    }

    QStringList FileOpenPlan::filePaths() const {
        Q_D(const FileOpenPlan);
        return d->request->filePaths;
    }

    bool FileOpenPlan::submit() {
        Q_D(FileOpenPlan);
        Q_ASSERT(QThread::currentThread() == thread());
        if (!isValid() || !d->acceptsDrop) {
            return false;
        }
        auto dispatcher = d->dispatcher;
        auto request = d->request;
        d->submitted = true;
        Q_EMIT validChanged();
        return dispatcher && dispatcher->d_func()->submit(request);
    }

    void FileOpenPlan::discard() {
        Q_ASSERT(QThread::currentThread() == thread());
        deleteLater();
        invalidate();
    }

    void FileOpenPlan::invalidate() {
        Q_D(FileOpenPlan);
        if (d->invalidated || d->submitted) {
            return;
        }
        d->invalidated = true;
        Q_EMIT validChanged();
    }

}

#include "moc_FileOpenPlan.cpp"
