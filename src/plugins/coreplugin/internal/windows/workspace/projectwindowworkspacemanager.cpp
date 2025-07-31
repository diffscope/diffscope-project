#include "projectwindowworkspacemanager.h"

#include <algorithm>
#include <iterator>
#include <ranges>

#include <CoreApi/plugindatabase.h>

namespace Core::Internal {

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
    void ProjectWindowWorkspaceManager::setCurrentLayout(const ProjectWindowWorkspaceLayout &layout) {
        m_currentLayout = layout;
        emit currentLayoutChanged();
    }
    QList<ProjectWindowWorkspaceLayout> ProjectWindowWorkspaceManager::customLayouts() {
        return m_customLayouts;
    }
    void ProjectWindowWorkspaceManager::setCustomLayouts(const QList<ProjectWindowWorkspaceLayout> &layouts) {
        auto oldLayouts = m_customLayouts;
        m_customLayouts = layouts;
        for (auto manager : m_instances) {
            if (manager->currentIndex() >= 0) {
                auto selectedCustomLayout = oldLayouts.value(manager->currentIndex());
                if (auto i = layouts.indexOf(selectedCustomLayout); i == -1) {
                    manager->setCurrentIndex(Invalid);
                } else {
                    manager->setCurrentIndex(i);
                }
            }
            emit manager->customLayoutsChanged();
        }
    }
    int ProjectWindowWorkspaceManager::currentIndex() const {
        return m_currentIndex;
    }
    void ProjectWindowWorkspaceManager::setCurrentIndex(int index) {
        if (m_currentIndex == index) {
            return;
        }
        m_currentIndex = index;
        emit currentIndexChanged();
    }
    ProjectWindowWorkspaceLayout ProjectWindowWorkspaceManager::defaultLayout() {
        ProjectWindowWorkspaceLayout layout;
        layout.setName("_default");
        layout.setViewSpec(ProjectWindowData::LeftBottom, {
            {{"core.settings", false}}, 400, 400, 0
        });
        layout.setViewSpec(ProjectWindowData::TopLeft, {
            {{"core.arrangementPanel", true}}, 600, 400, 0
        });
        layout.setViewSpec(ProjectWindowData::BottomLeft, {
            {{"core.pianoRollPanel", true}}, 400, 400, 0
        });
        layout.setViewSpec(ProjectWindowData::RightTop, {
            {{"core.notificationsPanel", true}}, 400, 400, -1
        });
        return layout;
    }

    static const char settingCategoryC[] = "WorkspaceManager";

    void ProjectWindowWorkspaceManager::load() {
        auto settings = PluginDatabase::settings();
        auto map = settings->value(settingCategoryC).toMap();
        if (!map.contains("currentLayout") || !map.contains("customLayouts") || !map.contains("currentIndex")) {
            return;
        }
        auto currentLayout = ProjectWindowWorkspaceLayout::fromVariant(map.value("currentLayout"));
        if (!currentLayout.isValid() || currentLayout.name() == "_default") {
            currentLayout = defaultLayout();
        }
        auto customLayoutsVariants = map.value("customLayouts").toList();
        QList<ProjectWindowWorkspaceLayout> customLayouts;
        std::ranges::transform(customLayoutsVariants | std::views::filter([](const auto &v) { return v.isValid(); }), std::back_inserter(customLayouts), [](const auto &v) {
            return ProjectWindowWorkspaceLayout::fromVariant(v);
        });
        auto currentIndex = map.value("currentIndex").toInt();
        if (currentIndex < Invalid || currentIndex >= customLayouts.size()) {
            currentIndex = Invalid;
        }
        setCurrentLayout(currentLayout);
        setCurrentIndex(currentIndex);
        setCustomLayouts(customLayouts);
    }
    void ProjectWindowWorkspaceManager::save() const {
        auto settings = PluginDatabase::settings();
        QVariantList customLayoutsVariants;
        std::ranges::transform(m_customLayouts, std::back_inserter(customLayoutsVariants), [](const auto &v) {
            return v.toVariant();
        });
        settings->setValue(settingCategoryC, QVariantMap {
            {"currentLayout", m_currentLayout.toVariant()},
            {"customLayouts", customLayoutsVariants},
            {"currentIndex", m_currentIndex},
        });
    }
}