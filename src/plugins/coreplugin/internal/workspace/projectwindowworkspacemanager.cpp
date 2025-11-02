#include "projectwindowworkspacemanager.h"

#include <algorithm>
#include <iterator>
#include <ranges>

#include <QLoggingCategory>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

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
            emit manager->customLayoutsChanged();
        }
    }
    ProjectWindowWorkspaceLayout ProjectWindowWorkspaceManager::defaultLayout() {
        ProjectWindowWorkspaceLayout layout;
        layout.setName("_default");
        layout.setViewSpec(
            ProjectWindowWorkspaceLayout::LeftTop,
            {{
                 {"core.panel.properties", true},
                 {"core.panel.metadata", true},
                 {"core.panel.plugins", true},
             },
             400,
             400,
             0
            }
        );
        layout.setViewSpec(
            ProjectWindowWorkspaceLayout::LeftBottom,
            {{{"core.panel.tips", true},
              {"core.settings", false}
             },
             400,
             150,
             0
            }
        );
        layout.setViewSpec(
            ProjectWindowWorkspaceLayout::TopLeft,
            {{{"core.panel.arrangement", true}
             },
             400,
             360,
             0
            }
        );
        layout.setViewSpec(
            ProjectWindowWorkspaceLayout::TopRight,
            {{}, 400, 360, -1
            }
        );
        layout.setViewSpec(
            ProjectWindowWorkspaceLayout::BottomLeft,
            {{{"core.panel.pianoRoll", true},
              {"core.panel.mixer", true}
             },
             400,
             640,
             0
            }
        );
        layout.setViewSpec(
            ProjectWindowWorkspaceLayout::BottomRight,
            {{}, 400, 640, -1
            }
        );
        layout.setViewSpec(
            ProjectWindowWorkspaceLayout::RightTop,
            {{{"core.panel.notifications", true}
             },
             400,
             400,
             -1
            }
        );
        layout.setViewSpec(
            ProjectWindowWorkspaceLayout::RightBottom,
            {{}, 400, 400, -1
            }
        );
        return layout;
    }

    void ProjectWindowWorkspaceManager::load() {
        qCDebug(lcProjectWindowWorkspaceManager) << "Loading settings";
        auto settings = RuntimeInterface::settings();
        auto map = settings->value(staticMetaObject.className()).toMap();
        if (!map.contains("currentLayout") || !map.contains("customLayouts")) {
            qCDebug(lcProjectWindowWorkspaceManager) << "No settings found, using default layout";
            setCurrentLayout(defaultLayout());
            return;
        }
        auto currentLayout = ProjectWindowWorkspaceLayout::fromVariant(map.value("currentLayout"));
        if (!currentLayout.isValid()) {
            qCWarning(lcProjectWindowWorkspaceManager) << "Invalid current layout, using default layout";
            currentLayout = defaultLayout();
        }
        auto customLayoutsVariants = map.value("customLayouts").toList();
        QList<ProjectWindowWorkspaceLayout> customLayouts;
        std::ranges::transform(customLayoutsVariants | std::views::filter([](const auto &v) { return v.isValid(); }), std::back_inserter(customLayouts), [](const auto &v) {
            return ProjectWindowWorkspaceLayout::fromVariant(v);
        });
        setCurrentLayout(currentLayout);
        setCustomLayouts(customLayouts);
    }
    void ProjectWindowWorkspaceManager::save() const {
        qCDebug(lcProjectWindowWorkspaceManager) << "Saving settings";
        auto settings = RuntimeInterface::settings();
        QVariantList customLayoutsVariants;
        std::ranges::transform(m_customLayouts, std::back_inserter(customLayoutsVariants), [](const auto &v) {
            return v.toVariant();
        });
        settings->setValue(
            staticMetaObject.className(),
            QVariantMap{
                {"currentLayout", m_currentLayout.toVariant()},
                {"customLayouts", customLayoutsVariants},
            }
        );
    }
}

#include "moc_projectwindowworkspacemanager.cpp"
