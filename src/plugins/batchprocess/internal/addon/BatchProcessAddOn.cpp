// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BatchProcessAddOn.h"

#include <memory>
#include <utility>

#include <QAbstractItemModel>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QScopedValueRollback>
#include <QSortFilterProxyModel>
#include <QUrl>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/DockingContext.h>
#include <SVSCraftQuick/DockingPanel.h>

#include <coreplugin/ActionWindowInterfaceBase.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/WorkspacePanelRegistry.h>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/Script.h>
#include <batchprocess/ScriptAction.h>
#include <batchprocess/internal/BatchProcessRuntime.h>
#include <batchprocess/internal/ScriptPanel.h>
#include <batchprocess/private/BatchProcessInterface_p.h>

namespace BatchProcess::Internal {

    namespace {

        class ScriptActionFilterModel : public QSortFilterProxyModel {
        public:
            explicit ScriptActionFilterModel(bool includeProjectActions, QObject *parent = nullptr)
                : QSortFilterProxyModel(parent), m_includeProjectActions(includeProjectActions) {
            }

        protected:
            bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
                const auto model = sourceModel();
                if (!model || sourceParent.isValid()) {
                    return false;
                }

                const auto index = model->index(sourceRow, 0, sourceParent);
                if (!index.data(ScriptActionModel::SeparatorRole).toBool()) {
                    return m_includeProjectActions || !index.data(ScriptActionModel::RequiresProjectRole).toBool();
                }

                const auto acceptsAction = [this, model, &sourceParent](int row) {
                    const auto actionIndex = model->index(row, 0, sourceParent);
                    return !actionIndex.data(ScriptActionModel::SeparatorRole).toBool() &&
                           (m_includeProjectActions || !actionIndex.data(ScriptActionModel::RequiresProjectRole).toBool());
                };
                const auto hasAcceptedAction = [&acceptsAction](int first, int last) {
                    for (auto row = first; row < last; ++row) {
                        if (acceptsAction(row)) {
                            return true;
                        }
                    }
                    return false;
                };

                const auto rowCount = model->rowCount(sourceParent);
                bool hasSourceActionAfter = false;
                for (auto row = sourceRow + 1; row < rowCount; ++row) {
                    if (!model->index(row, 0, sourceParent).data(ScriptActionModel::SeparatorRole).toBool()) {
                        hasSourceActionAfter = true;
                        break;
                    }
                }

                const auto hasAcceptedActionBefore = hasAcceptedAction(0, sourceRow);
                if (!hasSourceActionAfter) {
                    return hasAcceptedActionBefore;
                }
                return hasAcceptedActionBefore && hasAcceptedAction(sourceRow + 1, rowCount);
            }

        private:
            bool m_includeProjectActions{};
        };

    }

    Q_STATIC_LOGGING_CATEGORY(lcBatchProcessAddOn, "diffscope.batchprocess.addon")

    BatchProcessAddOn::BatchProcessAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    BatchProcessAddOn::~BatchProcessAddOn() {
        clearScriptPanels();
    }

    QAbstractItemModel *BatchProcessAddOn::actionModel() const {
        return m_actionModel;
    }

    void BatchProcessAddOn::executeAction(QObject *actionObject) {
        const auto action = qobject_cast<BatchProcess::ScriptAction *>(actionObject);
        BatchProcessInterface::instance()->executeAction(action, windowHandle()->cast<Core::ActionWindowInterfaceBase>());
    }

    void BatchProcessAddOn::reloadScripts() {
        BatchProcessInterface::instance()->reloadScripts();
    }

    void BatchProcessAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        auto actionModel = new ScriptActionFilterModel(qobject_cast<Core::ProjectWindowInterface *>(windowInterface), this);
        actionModel->setSourceModel(BatchProcessInterface::instance()->d_func()->actionModel);
        m_actionModel = actionModel;
        qCDebug(lcBatchProcessAddOn) << "Installing Batch Process actions" << windowInterface;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.BatchProcess", "BatchProcessActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
        if (!object) {
            qFatal() << component.errorString();
        }
        object->setParent(this);
        QMetaObject::invokeMethod(object, "registerToContext", windowInterface->actionContext());
    }

    void BatchProcessAddOn::extensionsInitialized() {
        m_projectWindow = qobject_cast<Core::ProjectWindowInterface *>(windowHandle());
        if (!m_projectWindow) {
            return;
        }
        m_panelRegistry = m_projectWindow->workspacePanelRegistry();
        m_dockingContext = m_projectWindow->getFirstObject<SVS::DockingContext>();
        connect(BatchProcessInterface::instance(), &BatchProcessInterface::scriptsReloaded, this, &BatchProcessAddOn::refreshScriptPanels, Qt::QueuedConnection);
        refreshScriptPanels();
    }

    bool BatchProcessAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    void BatchProcessAddOn::refreshScriptPanels() {
        if (!m_projectWindow || m_projectWindow->isEffectivelyClosed()) {
            return;
        }
        const auto batchProcess = BatchProcessInterface::instance();
        QScopedValueRollback creatingPanel(batchProcess->d_func()->creatingPanel, true);
        clearScriptPanels();
        m_scriptPanelContext = new QObject(this);

        const auto scripts = batchProcess->scripts();
        for (const auto script : scripts) {
            const auto panels = script->panels();
            for (qsizetype index = 0; index < panels.size(); ++index) {
                const auto &panel = panels.at(index);
                const auto component = panel.qmlComponent.data();
                if (!component) {
                    continue;
                }
                if (component->isLoading()) {
                    connect(component, &QQmlComponent::statusChanged, m_scriptPanelContext, [this, script = QPointer(script), index, panel] {
                        createScriptPanel(script.data(), index, panel);
                    }, Qt::SingleShotConnection);
                } else if (component->isReady()) {
                    createScriptPanel(script, index, panel);
                }
            }
        }
    }

    void BatchProcessAddOn::createScriptPanel(Script *script, qsizetype index, const Script::Panel &panel) {
        const auto window = m_projectWindow;
        const QPointer context(m_scriptPanelContext);
        const auto component = panel.qmlComponent.data();
        if (!context || !script || !window || window->isEffectivelyClosed() || !component || !component->isReady()) {
            return;
        }

        QScopedValueRollback creatingPanel(BatchProcessInterface::instance()->d_func()->creatingPanel, true);
        const auto scriptId = script->metadata().id();
        const auto source = component->url();
        QString error;
        std::unique_ptr<QObject> object(ScriptPanel::create(component, script, window.data(), &error));
        if (!context || context.data() != m_scriptPanelContext || !window || window->isEffectivelyClosed()) {
            return;
        }
        if (!object) {
            ScriptPanel::reportError(scriptId, panel.name, source, error, window.data());
            return;
        }
        const auto wrapper = qobject_cast<ScriptPanel *>(object.get());
        if (!wrapper) {
            ScriptPanel::reportError(scriptId, panel.name, source, BatchProcessInterface::tr("The QML root object must be a ScriptPanel."), window.data());
            return;
        }
        const auto innerComponent = wrapper->panel();
        if (!innerComponent) {
            ScriptPanel::reportError(scriptId, panel.name, source, BatchProcessInterface::tr("The ScriptPanel does not provide a panel component."), window.data());
            return;
        }

        wrapper->setParent(context.data());
        object.release();

        const auto registerPanel = [this, context, window, scriptId, index, name = panel.name, description = panel.description, source,
                                    wrapper = QPointer(wrapper), innerComponent = QPointer(innerComponent)] {
            if (!context || context.data() != m_scriptPanelContext || !window || window->isEffectivelyClosed() || !wrapper) {
                return;
            }
            if (!innerComponent || !innerComponent->isReady()) {
                const auto error = innerComponent ? innerComponent->errorString().trimmed() : QString();
                wrapper->deleteLater();
                ScriptPanel::reportError(scriptId, name, source, error, window.data());
                return;
            }

            const auto id = QStringLiteral("org.diffscope.batchprocess.scriptpanel:%1,%2").arg(scriptId, QString::number(index));
            const Core::WorkspacePanelRegistry::Metadata metadata{
                name,
                description,
                QUrl(QStringLiteral("image://fluent-system-icons/script")),
                {},
            };
            m_registeredScriptPanels.insert(id, {wrapper, innerComponent});
            if (!m_panelRegistry || !m_panelRegistry->registerPanel(id, innerComponent.data(), metadata)) {
                m_registeredScriptPanels.remove(id);
                wrapper->deleteLater();
                ScriptPanel::reportError(scriptId, name, source, BatchProcessInterface::tr("The panel could not be registered in the workspace."), window.data());
            }
        };
        if (innerComponent->isLoading()) {
            connect(innerComponent, &QQmlComponent::statusChanged, context.data(), registerPanel, Qt::SingleShotConnection);
        } else {
            registerPanel();
        }
    }

    void BatchProcessAddOn::clearScriptPanels() {
        const std::unique_ptr<QObject> context(std::exchange(m_scriptPanelContext, nullptr));
        const auto panels = std::exchange(m_registeredScriptPanels, {});
        for (auto it = panels.cbegin(); it != panels.cend(); ++it) {
            const auto dockedPanel = m_dockingContext ? m_dockingContext->panel(it.key()) : nullptr;
            if (it->wrapper && dockedPanel && dockedPanel->contentItem()) {
                // The panel content retains bindings to the wrapper until destruction.
                it->wrapper->setParent(dockedPanel->contentItem());
            }
            if (m_panelRegistry && it->component && m_panelRegistry->component(it.key()) == it->component.data()) {
                m_panelRegistry->unregisterPanel(it.key());
            }
        }
    }

}

#include "moc_BatchProcessAddOn.cpp"
