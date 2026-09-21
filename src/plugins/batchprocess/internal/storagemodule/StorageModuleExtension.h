// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_STORAGEMODULEEXTENSION_H
#define DIFFSCOPE_BATCHPROCESS_STORAGEMODULEEXTENSION_H

#include <QHash>
#include <QJSValue>
#include <QObject>
#include <QPointer>
#include <QString>

#include <batchprocess/Storage.h>
#include <batchprocess/StorageTypes.h>

class QJSEngine;

namespace BatchProcess {

    class BatchProcessInterface;
    class ScriptExecutionContext;
    class StorageInterface;

}

namespace BatchProcess::Internal {

    class StorageModuleBridge;

    class StorageModuleExtension : public QObject {
        Q_OBJECT
    public:
        explicit StorageModuleExtension(BatchProcessInterface *batchProcessInterface, StorageInterface *storageInterface, QObject *parent = nullptr);
        ~StorageModuleExtension() override;

        QJSValue wrapStorage(ScriptExecutionContext *context, Storage *storage, StorageError *error) const;
        bool unwrapStorage(ScriptExecutionContext *context, const QJSValue &value, Storage **storage, StorageError *error) const;

    private:
        friend class StorageModuleBridge;

        void installIntoEngine(QJSEngine *engine, ScriptExecutionContext *context);
        QJSValue createModule(ScriptExecutionContext *context) const;
        QJSValue helper(ScriptExecutionContext *context, const QString &name, StorageError *error) const;

        Storage *sessionStorage(const QString &id, StorageError *error);
        Storage *persistentStorage(const QString &id, StorageError *error);
        bool commitPersistentStorage(const QString &id, const StorageItems &items, StorageError *error);
        void loadPersistentStorage();
        bool writePersistentStorage(const QMap<QString, StorageItems> &storages, StorageError *error) const;

        BatchProcessInterface *m_batchProcessInterface;
        StorageInterface *m_storageInterface;
        QHash<QJSEngine *, QJSValue> m_engineBundles;
        QHash<QString, QPointer<Storage>> m_sessionStorages;
        QHash<QString, QPointer<Storage>> m_persistentStorages;
        QMap<QString, StorageItems> m_persistentItems;
        QString m_persistentFilePath;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_STORAGEMODULEEXTENSION_H
