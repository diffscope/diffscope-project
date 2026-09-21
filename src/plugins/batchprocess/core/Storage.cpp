// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Storage.h"
#include "Storage_p.h"

#include <exception>
#include <utility>

#include <QScopedValueRollback>

namespace BatchProcess {

    StoragePrivate::StoragePrivate(Storage *q) : q_ptr(q) {
    }

    void StoragePrivate::setError(StorageError *error, StorageErrorCode code, const QString &operation, const std::optional<QString> &key, const QString &message) const {
        if (!error) {
            return;
        }
        error->code = code;
        error->message = message;
        error->operation = operation;
        error->storageId = id;
        error->key = key;
    }

    bool StoragePrivate::commit(const StorageItems &proposedItems, const QString &operation, const std::optional<QString> &key, StorageError *error) {
        if (error) {
            error->clear();
        }
        if (committing) {
            setError(error, StorageErrorCode::InvalidState, operation, key, Storage::tr("A storage commit callback cannot modify the same storage."));
            return false;
        }
        if (!commitCallback) {
            items = proposedItems;
            return true;
        }

        StorageError localError;
        auto targetError = error ? error : &localError;
        QScopedValueRollback guard(committing, true);
        bool committed = false;
        try {
            committed = commitCallback(proposedItems, targetError);
        } catch (const std::exception &exception) {
            setError(targetError, StorageErrorCode::Unknown, operation, key, Storage::tr("The storage commit callback threw an exception: %1").arg(QString::fromUtf8(exception.what())));
            return false;
        } catch (...) {
            setError(targetError, StorageErrorCode::Unknown, operation, key, Storage::tr("The storage commit callback threw an unknown exception."));
            return false;
        }
        if (!committed) {
            if (!targetError->hasError()) {
                setError(targetError, StorageErrorCode::Unknown, operation, key, Storage::tr("The storage commit callback rejected the change."));
            } else {
                if (targetError->operation.isEmpty()) {
                    targetError->operation = operation;
                }
                if (targetError->storageId.isEmpty()) {
                    targetError->storageId = id;
                }
                if (!targetError->key && key) {
                    targetError->key = key;
                }
            }
            return false;
        }

        items = proposedItems;
        if (error) {
            error->clear();
        }
        return true;
    }

    Storage::Storage(const QString &id, const StorageItems &initialItems, StorageCommitCallback commitCallback, QObject *parent)
        : QObject(parent), d_ptr(new StoragePrivate(this)) {
        Q_D(Storage);
        d->id = id;
        d->items = initialItems;
        d->commitCallback = std::move(commitCallback);
    }

    Storage::~Storage() = default;

    QString Storage::id() const {
        Q_D(const Storage);
        return d->id;
    }

    qsizetype Storage::length() const {
        Q_D(const Storage);
        return d->items.size();
    }

    QStringList Storage::keys() const {
        Q_D(const Storage);
        return d->items.keys();
    }

    std::optional<QString> Storage::key(qsizetype index) const {
        Q_D(const Storage);
        if (index < 0 || index >= d->items.size()) {
            return std::nullopt;
        }
        return d->items.keys().at(index);
    }

    std::optional<QString> Storage::getItem(const QString &key) const {
        Q_D(const Storage);
        const auto it = d->items.constFind(key);
        if (it == d->items.cend()) {
            return std::nullopt;
        }
        return it.value();
    }

    StorageItems Storage::items() const {
        Q_D(const Storage);
        return d->items;
    }

    bool Storage::setItem(const QString &key, const QString &value, StorageError *error) {
        Q_D(Storage);
        const auto it = d->items.constFind(key);
        if (it != d->items.cend() && it.value() == value) {
            if (error) {
                error->clear();
            }
            return true;
        }
        auto proposedItems = d->items;
        proposedItems.insert(key, value);
        return d->commit(proposedItems, QStringLiteral("setItem"), key, error);
    }

    bool Storage::removeItem(const QString &key, StorageError *error) {
        Q_D(Storage);
        if (!d->items.contains(key)) {
            if (error) {
                error->clear();
            }
            return true;
        }
        auto proposedItems = d->items;
        proposedItems.remove(key);
        return d->commit(proposedItems, QStringLiteral("removeItem"), key, error);
    }

    bool Storage::clear(StorageError *error) {
        Q_D(Storage);
        if (d->items.isEmpty()) {
            if (error) {
                error->clear();
            }
            return true;
        }
        return d->commit({}, QStringLiteral("clear"), std::nullopt, error);
    }

}

#include "moc_Storage.cpp"
