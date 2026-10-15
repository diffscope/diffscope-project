// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WorkspaceAddOn.h"

#include <algorithm>
#include <ranges>

#include <QDebug>
#include <QQmlComponent>
#include <QSet>
#include <QSignalBlocker>
#include <QStandardItemModel>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftQuick/DockingContext.h>
#include <SVSCraftQuick/DockingPanel.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/ProjectWindowWorkspaceLayout.h>
#include <coreplugin/internal/ProjectWindowWorkspaceManager.h>
#include <coreplugin/internal/WorkspacePanelCatalog.h>
#include <coreplugin/internal/WorkspaceSession.h>

namespace Core::Internal {
    WorkspaceAddOn::WorkspaceAddOn(QObject *parent) : WindowInterfaceAddOn(parent), m_workspaceManager(new ProjectWindowWorkspaceManager(this)) {
        m_workspaceManager->load();
    }

    WorkspaceAddOn::~WorkspaceAddOn() {
        if (m_session)
            m_session->shutdown();
        delete m_actions.data();
        delete m_helper.data();
    }

    ProjectWindowWorkspaceManager *WorkspaceAddOn::workspaceManager() const {
        return m_workspaceManager;
    }

    WorkspaceSession *WorkspaceAddOn::session() const {
        return m_session;
    }

    void WorkspaceAddOn::initialize() {
        auto window = windowHandle()->cast<ProjectWindowInterface>();
        m_session = new WorkspaceSession(window, this);
        window->addObject(m_session);

        QQmlComponent helper(RuntimeInterface::qmlEngine(), "DiffScope.Core", "WorkspaceAddOnHelper");
        if (helper.isError()) {
            qFatal() << helper.errorString();
        }
        m_helper = helper.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
        if (!m_helper) {
            qFatal() << "WorkspaceAddOnHelper could not be created." << helper.errorString();
        }
        m_helper->setParent(this);

        QQmlComponent actions(RuntimeInterface::qmlEngine(), "DiffScope.Core", "WorkspaceAddOnActions");
        if (actions.isError()) {
            qFatal() << actions.errorString();
        }
        auto object = actions.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}, {"helper", QVariant::fromValue(m_helper.data())}});
        if (!object) {
            qFatal() << "WorkspaceAddOnActions could not be created." << actions.errorString();
        }
        object->setParent(this);
        m_actions = object;
        QMetaObject::invokeMethod(object, "registerToContext", window->actionContext());

        const std::pair<const char *, const char *> components[]{
            {"org.diffscope.core.panel.plugins", "PluginsPanel"},
            {"org.diffscope.core.panel.singers", "SingerPickerPanel"},
            {"org.diffscope.core.panel.tips", "TipsPanel"}
        };
        for (const auto &[id, type] : components) {
            auto component = new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", type, this);
            if (component->isError()) {
                qFatal() << component->errorString();
            }
            window->actionContext()->addAction(id, component);
        }
        connect(m_session->context(), &SVS::DockingContext::changed, this, [this] {
            if (!m_session->isBusy() && !m_session->isShuttingDown())
                Q_EMIT panelEntriesChanged();
        });
        connect(m_session, &WorkspaceSession::restored, this, &WorkspaceAddOn::panelEntriesChanged);
        connect(m_workspaceManager, &ProjectWindowWorkspaceManager::currentLayoutChanged, this, [this] {
            m_session->apply(m_workspaceManager->currentLayout());
        });
    }

    void WorkspaceAddOn::extensionsInitialized() {
        if (auto catalog = RuntimeInterface::instance()->getFirstObject<WorkspacePanelCatalog>())
            for (const auto &entry : catalog->entries())
                m_registeredPanelIds.append(entry.toMap().value("id").toString());
        Q_EMIT registeredPanelIdsChanged();
        m_session->initialize();
        Q_EMIT panelEntriesChanged();
        m_session->apply(m_workspaceManager->currentLayout());

        windowHandle()->addCloseCallback([this] {
            if (m_session->isBusy())
                return false;
            saveCurrentLayout();
            m_workspaceManager->save();
            return true;
        });
    }

    void WorkspaceAddOn::saveCurrentLayout() {
        if (!m_session || m_session->isShuttingDown())
            return;
        auto layout = m_session->snapshot(m_workspaceManager->currentLayout().name());
        QSignalBlocker blocker(m_workspaceManager);
        m_workspaceManager->setCurrentLayout(layout);
    }

    void WorkspaceAddOn::markNewPanelsPresented() {
        if (auto catalog = RuntimeInterface::instance()->getFirstObject<WorkspacePanelCatalog>())
            catalog->markPresented();
        Q_EMIT panelEntriesChanged();
    }

    void WorkspaceAddOn::saveCustomLayoutFromJavaScript(const ProjectWindowWorkspaceLayout &layout_, const QString &originName, const QString &name) const {
        auto layout = layout_;
        layout.setName(name);
        auto customLayouts = ProjectWindowWorkspaceManager::customLayouts();
        if (originName.isEmpty()) {
            auto it = std::ranges::find_if(customLayouts, [&](const auto &o) { return o.name() == name; });
            if (it != customLayouts.end()) {
                *it = layout;
            } else {
                customLayouts.append(layout);
            }
        } else {
            customLayouts.removeIf([&](const auto &o) { return o.name() == name; });
            auto it = std::ranges::find_if(customLayouts, [&](const auto &o) { return o.name() == originName; });
            if (it != customLayouts.end()) {
                *it = layout;
            }
        }
        ProjectWindowWorkspaceManager::setCustomLayouts(customLayouts);
        m_workspaceManager->save();
    }

    void WorkspaceAddOn::removeCustomLayoutFromJavaScript(const QString &name) const {
        auto customLayouts = ProjectWindowWorkspaceManager::customLayouts();
        customLayouts.removeIf([&](const auto &o) { return o.name() == name; });
        ProjectWindowWorkspaceManager::setCustomLayouts(customLayouts);
        m_workspaceManager->save();
    }

    void WorkspaceAddOn::showWorkspaceLayoutCommand() const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QStandardItemModel model;
        auto saveCurrentLayoutAsItem = new QStandardItem;
        saveCurrentLayoutAsItem->setData(tr("Save Current Layout As..."), SVS::SVSCraft::CP_TitleRole);
        model.appendRow(saveCurrentLayoutAsItem);
        auto separatorItem = new QStandardItem;
        separatorItem->setData(true, SVS::SVSCraft::CP_IsSeparatorRole);
        model.appendRow(separatorItem);
        auto defaultLayoutItem = new QStandardItem;
        defaultLayoutItem->setData(tr("Default Layout"), SVS::SVSCraft::CP_TitleRole);
        model.appendRow(defaultLayoutItem);
        if (!ProjectWindowWorkspaceManager::customLayouts().isEmpty()) {
            auto separatorItem = new QStandardItem;
            separatorItem->setData(true, SVS::SVSCraft::CP_IsSeparatorRole);
            model.appendRow(separatorItem);
        }
        for (const auto &layout : ProjectWindowWorkspaceManager::customLayouts()) {
            auto item = new QStandardItem;
            item->setData(layout.name(), SVS::SVSCraft::CP_TitleRole);
            item->setData(tr("custom layout"), SVS::SVSCraft::CP_TagRole);
            item->setData(QVariant::fromValue(layout), Qt::DisplayRole);
            model.appendRow(item);
        }
        auto index = windowInterface->execQuickPick(&model, tr("Workspace layout actions"));
        if (index == -1)
            return;
        if (index == 0) {
            windowInterface->triggerAction("org.diffscope.core.workspace.saveLayout");
        } else if (index == 2) {
            windowInterface->triggerAction("org.diffscope.core.workspace.defaultLayout");
        } else {
            auto layout =
                model.item(index)->data(Qt::DisplayRole).value<ProjectWindowWorkspaceLayout>();
            if (!layout.isValid())
                return;
            showWorkspaceCustomLayoutCommand(layout);
        }
    }

    void WorkspaceAddOn::showWorkspaceCustomLayoutCommand(const ProjectWindowWorkspaceLayout &layout) const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QStandardItemModel model;
        auto applyItem = new QStandardItem;
        applyItem->setData(tr("Apply"), SVS::SVSCraft::CP_TitleRole);
        model.appendRow(applyItem);
        auto renameItem = new QStandardItem;
        renameItem->setData(tr("Rename"), SVS::SVSCraft::CP_TitleRole);
        model.appendRow(renameItem);
        auto deleteItem = new QStandardItem;
        deleteItem->setData(tr("Delete"), SVS::SVSCraft::CP_TitleRole);
        model.appendRow(deleteItem);
        auto index = windowInterface->execQuickPick(&model, tr("Custom layout \"%1\" actions").arg(layout.name()));
        if (index == -1)
            return;
        switch (index) {
            case 0: {
                m_workspaceManager->setCurrentLayout(layout);
                break;
            }
            case 1: {
                QMetaObject::invokeMethod(m_helper, "promptSaveLayout", QVariant::fromValue(layout), QVariant::fromValue(layout.name()));
                break;
            }
            case 2: {
                QMetaObject::invokeMethod(m_helper, "promptDeleteLayout", QVariant::fromValue(layout.name()));
                break;
            }
        }
    }

    QVariantList WorkspaceAddOn::panelEntries() const {
        auto catalog = RuntimeInterface::instance()->getFirstObject<WorkspacePanelCatalog>();
        QVariantList result = catalog ? catalog->entries() : QVariantList{};
        if (!m_session)
            return result;
        QSet<QString> included;
        for (auto &entry : result) {
            auto map = entry.toMap();
            auto panel = m_session->context()->panel(map.value("id").toString());
            included.insert(map.value("id").toString());
            map.insert("panel", QVariant::fromValue(panel));
            map.insert("available", true);
            entry = map;
        }
        for (const auto &value : m_session->context()->panelObjects()) {
            auto panel = value.value<SVS::DockingPanel *>();
            if (!included.contains(panel->key()) && panel->group())
                result.append(QVariantMap{{"id", panel->key()}, {"text", panel->title()}, {"iconSource", panel->iconSource()}, {"iconColor", panel->iconColor()}, {"panel", QVariant::fromValue(panel)}, {"available", false}});
        }
        return result;
    }

    QVariantList WorkspaceAddOn::newPanelEntries() const {
        auto catalog = RuntimeInterface::instance()->getFirstObject<WorkspacePanelCatalog>();
        return catalog ? catalog->pendingEntries() : QVariantList{};
    }

    QStringList WorkspaceAddOn::registeredPanelIds() const {
        return m_registeredPanelIds;
    }
}

#include "moc_WorkspaceAddOn.cpp"
