// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_BATCHPROCESSINTERFACE_H
#define DIFFSCOPE_BATCHPROCESS_BATCHPROCESSINTERFACE_H

#include <functional>

#include <QJSValue>
#include <QList>
#include <QObject>
#include <QString>

#include <batchprocess/batchprocessglobal.h>

class QJSEngine;

namespace Core {
    class ActionWindowInterfaceBase;
}

namespace BatchProcess {

    namespace Internal {
        class BatchProcessAddOn;
        class BatchProcessPlugin;
        class BatchProcessRuntime;
    }

    class BatchProcessInterfacePrivate;
    class Script;
    class ScriptAction;
    class ScriptExecutionContext;

    class BATCH_PROCESS_EXPORT BatchProcessInterface : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(BatchProcessInterface)
    public:
        using BuiltinScriptFactory = std::function<QJSValue(ScriptExecutionContext *context)>;
        using ModuleFactory = std::function<QJSValue(ScriptExecutionContext *context)>;
        using GlobalObjectFactory = std::function<QJSValue(ScriptExecutionContext *context)>;
        using EngineExtension = std::function<void(QJSEngine *engine, ScriptExecutionContext *context)>;
        using ActionCallback = std::function<void(ScriptExecutionContext *context)>;

        ~BatchProcessInterface() override;

        static BatchProcessInterface *instance();

        /** Creates a built-in script in each newly created engine. The factory returns an object from that engine which is accepted by defineScript(). */
        bool registerBuiltinScript(QObject *owner, BuiltinScriptFactory factory);
        /** Registrations are replayed when the next engine is created. File-like and operating-system absolute names are rejected. */
        bool registerModule(const QString &name, QObject *owner, ModuleFactory factory);
        /** Installs a read-only property on each newly created engine global object. */
        bool registerGlobalObject(const QString &name, QObject *owner, GlobalObjectFactory factory);
        bool registerEngineExtension(QObject *owner, EngineExtension extension);
        bool registerActionStartedCallback(QObject *owner, ActionCallback callback);
        bool registerActionFinishedCallback(QObject *owner, ActionCallback callback);

        /** Returned objects remain valid only until the next successful reload. */
        QList<Script *> scripts() const;
        /** Returns the built-in or user script while it is loading or while one of its actions is executing. */
        Script *currentScript() const;
        ScriptExecutionContext *executionContext() const;

        bool reloadScripts();
        bool executeAction(ScriptAction *action, Core::ActionWindowInterfaceBase *windowInterface = nullptr);
        /** Requests a one-shot interruption of the currently executing script action. This method may be called from another thread. */
        void interruptCurrentExecution();

    private:
        friend class Internal::BatchProcessAddOn;
        friend class Internal::BatchProcessPlugin;
        friend class Internal::BatchProcessRuntime;

        explicit BatchProcessInterface(QObject *parent = nullptr);

        QScopedPointer<BatchProcessInterfacePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_BATCHPROCESSINTERFACE_H
