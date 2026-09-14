// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "FileSystemHandle.h"
#include "FileSystemHandle_p.h"

#include <QDir>

#include <batchprocess/ScriptAction.h>
#include <batchprocess/ScriptExecutionContext.h>

namespace BatchProcess {

    FileSystemHandle::FileSystemHandle() = default;

    FileSystemHandle::FileSystemHandle(FileSystemHandlePrivate *d) : d(d) {
    }

    FileSystemHandle::FileSystemHandle(const FileSystemHandle &other) = default;

    FileSystemHandle::FileSystemHandle(FileSystemHandle &&other) noexcept = default;

    FileSystemHandle::~FileSystemHandle() = default;

    FileSystemHandle &FileSystemHandle::operator=(const FileSystemHandle &other) = default;

    FileSystemHandle &FileSystemHandle::operator=(FileSystemHandle &&other) noexcept = default;

    bool FileSystemHandle::isValid() const {
        return d && d->kind != Invalid && d->token && d->token->active && d->token->engine && d->token->context && d->token->action && d->token->context->engine() == d->token->engine && d->token->context->action() == d->token->action;
    }

    FileSystemHandle::Kind FileSystemHandle::kind() const {
        return d ? d->kind : Invalid;
    }

    QString FileSystemHandle::path() const {
        return d ? QDir::toNativeSeparators(d->path) : QString();
    }

    FileAccessMode FileSystemHandle::access() const {
        return d ? d->access : FileAccessMode::Read;
    }

    bool FileSystemHandle::recursive() const {
        return d && d->recursive;
    }

}
