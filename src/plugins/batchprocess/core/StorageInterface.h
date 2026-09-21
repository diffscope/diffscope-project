// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_STORAGEINTERFACE_H
#define DIFFSCOPE_BATCHPROCESS_STORAGEINTERFACE_H

#include <QJSValue>
#include <QObject>
#include <QScopedPointer>

#include <batchprocess/StorageTypes.h>
#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    namespace Internal {
        class BatchProcessPlugin;
        class StorageModuleExtension;
    }

    class ScriptExecutionContext;
    class Storage;
    class StorageInterfacePrivate;

    class BATCH_PROCESS_EXPORT StorageInterface : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(StorageInterface)
    public:
        ~StorageInterface() override;

        static StorageInterface *instance();

        /** Creates an independently owned storage initialized with a snapshot of string values. */
        Storage *createStorage(const QString &id, const StorageItems &initialItems, QObject *parent, StorageCommitCallback commitCallback = {});

        /** Returns the engine-specific frozen JavaScript wrapper for storage. */
        QJSValue toJavaScriptValue(ScriptExecutionContext *context, Storage *storage, StorageError *error = nullptr);
        /** Resolves a live Storage wrapped for the same JavaScript engine as context. */
        bool fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, Storage **storage, StorageError *error = nullptr) const;

    private:
        friend class Internal::BatchProcessPlugin;
        friend class Internal::StorageModuleExtension;

        explicit StorageInterface(QObject *parent = nullptr);

        QScopedPointer<StorageInterfacePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_STORAGEINTERFACE_H
