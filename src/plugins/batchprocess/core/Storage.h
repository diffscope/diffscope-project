// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_STORAGE_H
#define DIFFSCOPE_BATCHPROCESS_STORAGE_H

#include <optional>

#include <QObject>
#include <QScopedPointer>
#include <QStringList>

#include <batchprocess/StorageTypes.h>
#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    class StorageInterface;
    class StoragePrivate;

    class BATCH_PROCESS_EXPORT Storage : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Storage)
    public:
        ~Storage() override;

        QString id() const;
        qsizetype length() const;
        QStringList keys() const;
        std::optional<QString> key(qsizetype index) const;
        std::optional<QString> getItem(const QString &key) const;
        StorageItems items() const;

        bool setItem(const QString &key, const QString &value, StorageError *error = nullptr);
        bool removeItem(const QString &key, StorageError *error = nullptr);
        bool clear(StorageError *error = nullptr);

    private:
        friend class StorageInterface;

        Storage(const QString &id, const StorageItems &initialItems, StorageCommitCallback commitCallback, QObject *parent = nullptr);

        QScopedPointer<StoragePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_STORAGE_H
