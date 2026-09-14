// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BatchProcessInterface.h"
#include "BatchProcessInterface_p.h"

#include <algorithm>
#include <utility>

#include <QDir>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QMutexLocker>

#include <batchprocess/Script.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/internal/BatchProcessRuntime.h>
#include <batchprocess/internal/BatchProcessSettings.h>

namespace BatchProcess {

    Q_STATIC_LOGGING_CATEGORY(lcBatchProcessInterface, "diffscope.batchprocess.interface")

    static BatchProcessInterface *m_instance = nullptr;

    namespace {

        bool reservedGlobalName(const QString &name) {
            return name == QStringLiteral("defineScript") || name == QStringLiteral("require") || name == QStringLiteral("module");
        }

    }

    BatchProcessInterfacePrivate::BatchProcessInterfacePrivate(BatchProcessInterface *q) : q_ptr(q) {
        actionModel = new Internal::ScriptActionModel(q);
    }

    BatchProcessInterfacePrivate::~BatchProcessInterfacePrivate() = default;

    void BatchProcessInterfacePrivate::watchOwner(QObject *owner) {
        const auto alreadyWatched = std::ranges::any_of(watchedOwners, [owner](const QPointer<QObject> &item) {
            return item.data() == owner;
        });
        if (alreadyWatched) {
            return;
        }
        watchedOwners.append(owner);
        QObject::connect(owner, &QObject::destroyed, q_ptr, [this](QObject *destroyedOwner) {
            removeOwner(destroyedOwner);
        });
    }

    void BatchProcessInterfacePrivate::removeOwner(QObject *owner) {
        if (runtime) {
            runtime->removeOwner(owner);
        }
        if (loadingRuntime && loadingRuntime != runtime.get()) {
            loadingRuntime->removeOwner(owner);
        }
        auto belongsToOwner = [owner](const auto &registration) {
            return !registration.owner || registration.owner.data() == owner;
        };
        erase_if(builtinScriptRegistrations, belongsToOwner);
        erase_if(moduleRegistrations, belongsToOwner);
        erase_if(globalObjectRegistrations, belongsToOwner);
        erase_if(engineExtensionRegistrations, belongsToOwner);
        erase_if(actionStartedCallbacks, belongsToOwner);
        erase_if(actionFinishedCallbacks, belongsToOwner);
        erase_if(watchedOwners, [owner](const QPointer<QObject> &item) {
            return !item || item.data() == owner;
        });
        qCDebug(lcBatchProcessInterface) << "Removed registrations for destroyed owner" << owner;
    }

    void BatchProcessInterfacePrivate::refreshActionModel() {
        actionModel->setScripts(runtime ? runtime->scripts() : QList<Script *>{});
    }

    void BatchProcessInterfacePrivate::beginExecution(QJSEngine *engine) {
        QMutexLocker locker(&interruptionMutex);
        Q_ASSERT(engine);
        Q_ASSERT(!executingEngine);
        engine->setInterrupted(false);
        executingEngine = engine;
    }

    void BatchProcessInterfacePrivate::endExecution(QJSEngine *engine) {
        QMutexLocker locker(&interruptionMutex);
        Q_ASSERT(engine);
        Q_ASSERT(executingEngine == engine);
        executingEngine = nullptr;
        engine->setInterrupted(false);
    }

    void BatchProcessInterfacePrivate::interruptCurrentExecution() {
        QMutexLocker locker(&interruptionMutex);
        if (executingEngine) {
            executingEngine->setInterrupted(true);
            qCInfo(lcBatchProcessInterface) << "Requested interruption of the current script execution";
        } else {
            qCDebug(lcBatchProcessInterface) << "Ignored script interruption request because no script action is executing";
        }
    }

    BatchProcessInterface::BatchProcessInterface(QObject *parent)
        : QObject(parent), d_ptr(new BatchProcessInterfacePrivate(this)) {
        Q_ASSERT(!m_instance);
        m_instance = this;
    }

    BatchProcessInterface::~BatchProcessInterface() {
        Q_D(BatchProcessInterface);
        d->actionModel->setScripts({});
        d->runtime.reset();
        m_instance = nullptr;
    }

    BatchProcessInterface *BatchProcessInterface::instance() {
        return m_instance;
    }

    bool BatchProcessInterface::registerBuiltinScript(QObject *owner, BuiltinScriptFactory factory) {
        Q_D(BatchProcessInterface);
        if (!owner || !factory) {
            qCWarning(lcBatchProcessInterface) << "Rejected built-in script registration";
            return false;
        }
        d->builtinScriptRegistrations.append({owner, std::move(factory)});
        d->watchOwner(owner);
        qCDebug(lcBatchProcessInterface) << "Registered built-in script";
        return true;
    }

    bool BatchProcessInterface::registerModule(const QString &name, QObject *owner, ModuleFactory factory) {
        Q_D(BatchProcessInterface);
        if (!owner || !factory || name.trimmed().isEmpty() || name != name.trimmed() || name.startsWith(QLatin1Char('.')) || name.startsWith(QLatin1Char('/')) || QDir::isAbsolutePath(name)) {
            qCWarning(lcBatchProcessInterface) << "Rejected module registration" << name;
            return false;
        }
        if (std::ranges::any_of(d->moduleRegistrations, [&name](const auto &registration) {
                return registration.name == name;
            })) {
            qCWarning(lcBatchProcessInterface) << "Rejected duplicate module registration" << name;
            return false;
        }
        d->moduleRegistrations.append({name, owner, std::move(factory)});
        d->watchOwner(owner);
        qCDebug(lcBatchProcessInterface) << "Registered module" << name;
        return true;
    }

    bool BatchProcessInterface::registerGlobalObject(const QString &name, QObject *owner, GlobalObjectFactory factory) {
        Q_D(BatchProcessInterface);
        if (!owner || !factory || name.trimmed().isEmpty() || name != name.trimmed() || reservedGlobalName(name)) {
            qCWarning(lcBatchProcessInterface) << "Rejected global object registration" << name;
            return false;
        }
        if (std::ranges::any_of(d->globalObjectRegistrations, [&name](const auto &registration) {
                return registration.name == name;
            })) {
            qCWarning(lcBatchProcessInterface) << "Rejected duplicate global object registration" << name;
            return false;
        }
        d->globalObjectRegistrations.append({name, owner, std::move(factory)});
        d->watchOwner(owner);
        qCDebug(lcBatchProcessInterface) << "Registered global object" << name;
        return true;
    }

    bool BatchProcessInterface::registerEngineExtension(QObject *owner, EngineExtension extension) {
        Q_D(BatchProcessInterface);
        if (!owner || !extension) {
            qCWarning(lcBatchProcessInterface) << "Rejected engine extension registration";
            return false;
        }
        d->engineExtensionRegistrations.append({owner, std::move(extension)});
        d->watchOwner(owner);
        qCDebug(lcBatchProcessInterface) << "Registered engine extension";
        return true;
    }

    bool BatchProcessInterface::registerActionStartedCallback(QObject *owner, ActionCallback callback) {
        Q_D(BatchProcessInterface);
        if (!owner || !callback) {
            qCWarning(lcBatchProcessInterface) << "Rejected action-start callback registration";
            return false;
        }
        d->actionStartedCallbacks.append({owner, std::move(callback)});
        d->watchOwner(owner);
        qCDebug(lcBatchProcessInterface) << "Registered action-start callback";
        return true;
    }

    bool BatchProcessInterface::registerActionFinishedCallback(QObject *owner, ActionCallback callback) {
        Q_D(BatchProcessInterface);
        if (!owner || !callback) {
            qCWarning(lcBatchProcessInterface) << "Rejected action-finished callback registration";
            return false;
        }
        d->actionFinishedCallbacks.append({owner, std::move(callback)});
        d->watchOwner(owner);
        qCDebug(lcBatchProcessInterface) << "Registered action-finished callback";
        return true;
    }

    QList<Script *> BatchProcessInterface::scripts() const {
        Q_D(const BatchProcessInterface);
        if (!d->runtime) {
            return {};
        }
        return d->runtime->scripts();
    }

    Script *BatchProcessInterface::currentScript() const {
        Q_D(const BatchProcessInterface);
        const auto activeRuntime = d->loadingRuntime ? d->loadingRuntime : d->runtime.get();
        return activeRuntime ? activeRuntime->currentScript() : nullptr;
    }

    ScriptExecutionContext *BatchProcessInterface::executionContext() const {
        Q_D(const BatchProcessInterface);
        const auto activeRuntime = d->loadingRuntime ? d->loadingRuntime : d->runtime.get();
        return activeRuntime ? activeRuntime->executionContext() : nullptr;
    }

    bool BatchProcessInterface::reloadScripts() {
        Q_D(BatchProcessInterface);
        if (d->reloading || (d->runtime && d->runtime->currentScript())) {
            qCWarning(lcBatchProcessInterface) << "Rejected reentrant script reload";
            return false;
        }

        d->reloading = true;
        qCInfo(lcBatchProcessInterface) << "Reloading scripts from" << Internal::BatchProcessSettings::scriptDirectory();
        auto nextRuntime = std::make_unique<Internal::BatchProcessRuntime>(d);
        d->loadingRuntime = nextRuntime.get();
        const auto initialized = nextRuntime->initialize(Internal::BatchProcessSettings::scriptDirectory());
        d->loadingRuntime = nullptr;
        if (!initialized) {
            d->reloading = false;
            return false;
        }

        auto oldRuntime = std::move(d->runtime);
        d->runtime = std::move(nextRuntime);
        d->refreshActionModel();
        oldRuntime.reset();
        d->reloading = false;
        qCInfo(lcBatchProcessInterface) << "Scripts reloaded" << d->runtime->scripts().size();
        return true;
    }

    bool BatchProcessInterface::executeAction(ScriptAction *action, Core::ActionWindowInterfaceBase *windowInterface) {
        Q_D(BatchProcessInterface);
        if (d->reloading || !d->runtime) {
            qCWarning(lcBatchProcessInterface) << "Rejected script action execution while runtime is unavailable";
            return false;
        }
        return d->runtime->executeAction(action, windowInterface);
    }

    void BatchProcessInterface::interruptCurrentExecution() {
        Q_D(BatchProcessInterface);
        d->interruptCurrentExecution();
    }

}

#include "moc_BatchProcessInterface.cpp"
