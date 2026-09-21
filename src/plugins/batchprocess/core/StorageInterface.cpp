// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "StorageInterface.h"
#include "StorageInterface_p.h"

#include <utility>

#include <QLoggingCategory>

#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/Storage.h>
#include <batchprocess/internal/StorageModuleExtension.h>

namespace BatchProcess {

    Q_STATIC_LOGGING_CATEGORY(lcStorageInterface, "diffscope.batchprocess.storage.interface")

    namespace {

        StorageInterface *s_instance{};

    }

    StorageInterfacePrivate::StorageInterfacePrivate(StorageInterface *q) : q_ptr(q) {
    }

    void StorageInterfacePrivate::setError(StorageError *error, StorageErrorCode code, const QString &operation, const QString &storageId, const QString &message) {
        if (!error) {
            return;
        }
        error->code = code;
        error->message = message;
        error->operation = operation;
        error->storageId = storageId;
        error->key.reset();
    }

    StorageInterface::StorageInterface(QObject *parent) : QObject(parent), d_ptr(new StorageInterfacePrivate(this)) {
        Q_ASSERT(!s_instance);
        s_instance = this;
    }

    StorageInterface::~StorageInterface() {
        s_instance = nullptr;
    }

    StorageInterface *StorageInterface::instance() {
        return s_instance;
    }

    Storage *StorageInterface::createStorage(const QString &id, const StorageItems &initialItems, QObject *parent, StorageCommitCallback commitCallback) {
        return new Storage(id, initialItems, std::move(commitCallback), parent);
    }

    QJSValue StorageInterface::toJavaScriptValue(ScriptExecutionContext *context, Storage *storage, StorageError *error) {
        Q_D(StorageInterface);
        if (!context || !context->engine() || !storage || !d->moduleExtension) {
            StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("wrap"), storage ? storage->id() : QString(), tr("The storage cannot be wrapped in the current JavaScript execution context."));
            qCWarning(lcStorageInterface) << "Unable to wrap storage for JavaScript" << (storage ? storage->id() : QString());
            return QJSValue(QJSValue::UndefinedValue);
        }
        return d->moduleExtension->wrapStorage(context, storage, error);
    }

    bool StorageInterface::fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, Storage **storage, StorageError *error) const {
        Q_D(const StorageInterface);
        if (!storage) {
            StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("unwrap"), {}, tr("No storage output was provided."));
            return false;
        }
        *storage = nullptr;
        if (!context || !context->engine() || !d->moduleExtension) {
            StorageInterfacePrivate::setError(error, StorageErrorCode::InvalidState, QStringLiteral("unwrap"), {}, tr("The JavaScript execution context is not available."));
            qCWarning(lcStorageInterface) << "Unable to resolve JavaScript storage";
            return false;
        }
        return d->moduleExtension->unwrapStorage(context, value, storage, error);
    }

}

#include "moc_StorageInterface.cpp"
