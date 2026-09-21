// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_STORAGEINTERFACE_P_H
#define DIFFSCOPE_BATCHPROCESS_STORAGEINTERFACE_P_H

#include <batchprocess/StorageInterface.h>

#include <QPointer>

namespace BatchProcess {

    namespace Internal {
        class StorageModuleExtension;
    }

    class StorageInterfacePrivate {
        Q_DECLARE_PUBLIC(StorageInterface)
    public:
        explicit StorageInterfacePrivate(StorageInterface *q);

        static void setError(StorageError *error, StorageErrorCode code, const QString &operation, const QString &storageId, const QString &message);

        StorageInterface *q_ptr;
        QPointer<Internal::StorageModuleExtension> moduleExtension;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_STORAGEINTERFACE_P_H
