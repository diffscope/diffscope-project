// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_STORAGE_P_H
#define DIFFSCOPE_BATCHPROCESS_STORAGE_P_H

#include <batchprocess/Storage.h>

namespace BatchProcess {

    class StoragePrivate {
        Q_DECLARE_PUBLIC(Storage)
    public:
        explicit StoragePrivate(Storage *q);

        bool commit(const StorageItems &proposedItems, const QString &operation, const std::optional<QString> &key, StorageError *error);
        void setError(StorageError *error, StorageErrorCode code, const QString &operation, const std::optional<QString> &key, const QString &message) const;

        Storage *q_ptr;
        QString id;
        StorageItems items;
        StorageCommitCallback commitCallback;
        bool committing{};
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_STORAGE_P_H
