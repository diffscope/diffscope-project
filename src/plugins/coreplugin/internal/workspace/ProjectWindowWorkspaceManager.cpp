// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProjectWindowWorkspaceManager.h"

#include <algorithm>
#include <iterator>
#include <ranges>

#include <QLoggingCategory>
#include <QSettings>
#include <QVariantMap>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/DockingTypes.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcProjectWindowWorkspaceManager, "diffscope.core.projectwindowworkspacemanager")

    QSet<ProjectWindowWorkspaceManager *> m_instances;
    QList<ProjectWindowWorkspaceLayout> m_customLayouts;

    ProjectWindowWorkspaceManager::ProjectWindowWorkspaceManager(QObject *parent) : QObject(parent) {
        m_instances.insert(this);
    }

    ProjectWindowWorkspaceManager::~ProjectWindowWorkspaceManager() {
        m_instances.remove(this);
    }

    ProjectWindowWorkspaceLayout ProjectWindowWorkspaceManager::currentLayout() const {
        return m_currentLayout;
    }

    void ProjectWindowWorkspaceManager::setCurrentLayoutSnapshot(const ProjectWindowWorkspaceLayout &layout) {
        m_currentLayout = layout;
    }

    void ProjectWindowWorkspaceManager::applyDefaultLayout() {
        setCurrentLayout(defaultLayout(), false);
    }

    void ProjectWindowWorkspaceManager::applyCustomLayout(const ProjectWindowWorkspaceLayout &layout) {
        setCurrentLayout(layout, true);
    }

    void ProjectWindowWorkspaceManager::setCurrentLayout(const ProjectWindowWorkspaceLayout &layout, bool isCustom) {
        m_currentLayout = layout;
        m_currentLayoutIsCustom = isCustom;
        Q_EMIT currentLayoutChanged();
    }

    QList<ProjectWindowWorkspaceLayout> ProjectWindowWorkspaceManager::customLayouts() {
        return m_customLayouts;
    }

    void ProjectWindowWorkspaceManager::setCustomLayouts(const QList<ProjectWindowWorkspaceLayout> &layouts) {
        m_customLayouts = layouts;
        for (auto manager : m_instances) {
            Q_EMIT manager->customLayoutsChanged();
        }
    }

    ProjectWindowWorkspaceLayout ProjectWindowWorkspaceManager::defaultLayout() {
        using SVS::DockingNodeState;
        auto group = [](const QString &id, Qt::Edge edge, const QStringList &leading, const QStringList &trailing, bool open, bool locked) {
            DockingNodeState n;
            n.nodeId = id;
            n.tabBarEdge = edge;
            n.panelIds[0] = leading;
            n.selected[0] = open ? leading.value(0) : QString{};
            n.lastSelected[0] = leading.value(0);
            n.panelIds[1] = trailing;
            n.lastSelected[1] = trailing.value(0);
            n.widthLocked = locked;
            return n;
        };
        auto top = group("arrangement", Qt::TopEdge, {"org.diffscope.visualeditor.panel.arrangement"}, {}, true, false);
        top.showPanelNames = true;
        top.compactLayout = true;
        top.autoHide = true;
        auto bottom = group("editors", Qt::BottomEdge, {"org.diffscope.visualeditor.panel.pianoRoll", "org.diffscope.visualeditor.panel.mixer"}, {}, true, false);
        bottom.showPanelNames = true;
        bottom.compactLayout = true;
        auto right = group("right", Qt::RightEdge, {"org.diffscope.core.panel.properties", "org.diffscope.audio.panel.effects", "org.diffscope.core.panel.notifications", "org.diffscope.core.panel.plugins"}, {"org.diffscope.core.panel.singers", "org.diffscope.synth.panel.services", "org.diffscope.core.panel.recentFiles", "org.diffscope.core.panel.recoveryFiles"}, true, true);
        right.selected[1] = right.panelIds[1].value(0);
        right.lockedSize.setWidth(424);
        DockingNodeState center;
        center.kind = "split";
        center.nodeId = "center";
        center.orientation = Qt::Vertical;
        center.children = {top, bottom};
        center.weights = {2, 3};
        DockingNodeState root;
        root.kind = "split";
        root.nodeId = "root";
        root.children = {center, right};
        root.weights = {2, 1};
        ProjectWindowWorkspaceLayout layout;
        layout.setName("_default");
        layout.setState({
            {"mainRoot", root.toVariant()},
            {"floatingHosts", QVariantList{}},
            {"panelStates", QVariantMap{}},
            {"activePanelId", "org.diffscope.visualeditor.panel.arrangement"}
        });
        return layout;
    }

    void ProjectWindowWorkspaceManager::load() {
        qCDebug(lcProjectWindowWorkspaceManager) << "Loading settings";
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.core.workspace"));
        auto customLayoutsVariants = settings->value(QStringLiteral("customLayouts")).toList();
        const auto selection = settings->value(QStringLiteral("currentLayoutSelection")).toMap();
        settings->endGroup();

        auto state = RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.core.workspace"));
        const auto snapshot = state->value(QStringLiteral("currentLayout")).toMap();
        state->endGroup();

        QList<ProjectWindowWorkspaceLayout> customLayouts;
        std::ranges::transform(customLayoutsVariants, std::back_inserter(customLayouts), [](const auto &v) {
            return ProjectWindowWorkspaceLayout::fromVariant(v);
        });
        customLayouts.removeIf([](const auto &v) { return !v.isValid(); });

        auto currentLayout = ProjectWindowWorkspaceLayout::fromVariant(snapshot.value(QStringLiteral("layout")));
        auto isCustom = (currentLayout.isValid() ? snapshot : selection).value(QStringLiteral("isCustom")).toBool();
        if (!currentLayout.isValid() && isCustom) {
            const auto name = selection.value(QStringLiteral("name")).toString();
            const auto it = std::ranges::find_if(customLayouts, [&](const auto &layout) {
                return layout.name() == name;
            });
            if (it != customLayouts.end()) {
                currentLayout = *it;
            }
        }
        if (!currentLayout.isValid()) {
            qCDebug(lcProjectWindowWorkspaceManager) << "No valid current layout found, using default layout";
            currentLayout = defaultLayout();
            isCustom = false;
        }
        setCurrentLayout(currentLayout, isCustom);
        setCustomLayouts(customLayouts);
    }

    void ProjectWindowWorkspaceManager::save() const {
        qCDebug(lcProjectWindowWorkspaceManager) << "Saving settings";
        auto settings = RuntimeInterface::settings();
        QVariantList customLayoutsVariants;
        std::ranges::transform(m_customLayouts, std::back_inserter(customLayoutsVariants), [](const auto &v) {
            return v.toVariant();
        });
        settings->beginGroup(QStringLiteral("org.diffscope.core.workspace"));
        settings->setValue(QStringLiteral("customLayouts"), customLayoutsVariants);
        settings->setValue(QStringLiteral("currentLayoutSelection"), QVariantMap{
            {QStringLiteral("isCustom"), m_currentLayoutIsCustom},
            {QStringLiteral("name"), m_currentLayout.name()},
        });
        settings->endGroup();

        auto state = RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.core.workspace"));
        state->setValue(QStringLiteral("currentLayout"), QVariantMap{
            {QStringLiteral("isCustom"), m_currentLayoutIsCustom},
            {QStringLiteral("layout"), m_currentLayout.toVariant()},
        });
        state->endGroup();
    }
}

#include "moc_ProjectWindowWorkspaceManager.cpp"
