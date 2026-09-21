// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_STORAGETYPES_H
#define DIFFSCOPE_BATCHPROCESS_STORAGETYPES_H

#include <functional>
#include <optional>

#include <QMap>
#include <QString>

#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    using StorageItems = QMap<QString, QString>;

    enum class StorageErrorCode {
        None,
        InvalidState,
        Io,
        Unknown,
    };

    struct BATCH_PROCESS_EXPORT StorageError {
        StorageErrorCode code{StorageErrorCode::None};
        QString message;
        QString operation;
        QString storageId;
        std::optional<QString> key;

        bool hasError() const;
        void clear();
    };

    /** Persists a complete candidate snapshot. Returning false leaves the Storage unchanged. */
    using StorageCommitCallback = std::function<bool(const StorageItems &proposedItems, StorageError *error)>;

}

#endif // DIFFSCOPE_BATCHPROCESS_STORAGETYPES_H
