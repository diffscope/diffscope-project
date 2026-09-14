// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_BATCHPROCESSINTERFACE_P_H
#define DIFFSCOPE_BATCHPROCESS_BATCHPROCESSINTERFACE_P_H

#include <batchprocess/BatchProcessInterface.h>

#include <memory>

#include <QMutex>
#include <QPointer>
#include <QString>
#include <QVector>

namespace BatchProcess {

    namespace Internal {
        class BatchProcessRuntime;
        class ScriptActionModel;
    }

    class BatchProcessInterfacePrivate {
        Q_DECLARE_PUBLIC(BatchProcessInterface)
    public:
        struct BuiltinScriptRegistration {
            QPointer<QObject> owner;
            BatchProcessInterface::BuiltinScriptFactory factory;
        };

        struct ModuleRegistration {
            QString name;
            QPointer<QObject> owner;
            BatchProcessInterface::ModuleFactory factory;
        };

        struct GlobalObjectRegistration {
            QString name;
            QPointer<QObject> owner;
            BatchProcessInterface::GlobalObjectFactory factory;
        };

        struct EngineExtensionRegistration {
            QPointer<QObject> owner;
            BatchProcessInterface::EngineExtension extension;
        };

        struct ActionCallbackRegistration {
            QPointer<QObject> owner;
            BatchProcessInterface::ActionCallback callback;
        };

        explicit BatchProcessInterfacePrivate(BatchProcessInterface *q);
        ~BatchProcessInterfacePrivate();

        void watchOwner(QObject *owner);
        void removeOwner(QObject *owner);
        void refreshActionModel();
        void beginExecution(QJSEngine *engine);
        void endExecution(QJSEngine *engine);
        void interruptCurrentExecution();

        BatchProcessInterface *q_ptr;
        QVector<BuiltinScriptRegistration> builtinScriptRegistrations;
        QVector<ModuleRegistration> moduleRegistrations;
        QVector<GlobalObjectRegistration> globalObjectRegistrations;
        QVector<EngineExtensionRegistration> engineExtensionRegistrations;
        QVector<ActionCallbackRegistration> actionStartedCallbacks;
        QVector<ActionCallbackRegistration> actionFinishedCallbacks;
        QList<QPointer<QObject>> watchedOwners;
        QMutex interruptionMutex;
        QJSEngine *executingEngine{};
        std::unique_ptr<Internal::BatchProcessRuntime> runtime;
        Internal::BatchProcessRuntime *loadingRuntime{};
        Internal::ScriptActionModel *actionModel{};
        bool reloading{};
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_BATCHPROCESSINTERFACE_P_H
