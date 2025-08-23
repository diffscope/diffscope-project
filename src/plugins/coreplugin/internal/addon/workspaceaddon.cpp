#include "workspaceaddon.h"

#include <algorithm>
#include <ranges>

#include <QQmlComponent>
#include <QQmlEngine>

#include <QAKCore/actionregistry.h>
#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/iprojectwindow.h>
#include <coreplugin/internal/projectwindowworkspacemanager.h>
#include <coreplugin/internal/projectwindowworkspacelayout.h>

namespace Core::Internal {
    WorkspaceAddOn::WorkspaceAddOn(QObject *parent) : IWindowAddOn(parent) {
        m_workspaceManager = new ProjectWindowWorkspaceManager(this);
        m_workspaceManager->load();
    }
    WorkspaceAddOn::~WorkspaceAddOn() {
        m_workspaceManager->save();
    }
    void WorkspaceAddOn::initialize() {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        QObject *helper;
        {
            QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "WorkspaceAddOnHelper");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            helper = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)}
            });
            helper->setParent(iWin->window());
        }
        {
            QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "WorkspaceAddOnActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
                {"helper", QVariant::fromValue(helper)}
            });
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", iWin->actionContext());

            iWin->actionContext()->addAction("core.propertiesPanel", new QQmlComponent(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "PropertiesPanel", this));
            iWin->actionContext()->addAction("core.pluginsPanel", new QQmlComponent(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "PluginsPanel", this));
            iWin->actionContext()->addAction("core.tipsPanel", new QQmlComponent(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "TipsPanel", this));
            iWin->actionContext()->addAction("core.arrangementPanel", new QQmlComponent(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "ArrangementPanel", this));
            iWin->actionContext()->addAction("core.mixerPanel", new QQmlComponent(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "MixerPanel", this));
            iWin->actionContext()->addAction("core.pianoRollPanel", new QQmlComponent(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "PianoRollPanel", this));
        }
    }
    void WorkspaceAddOn::extensionsInitialized() {
    }
    bool WorkspaceAddOn::delayedInitialize() {
        emit panelEntriesChanged();
        return IWindowAddOn::delayedInitialize();
    }
    ProjectWindowWorkspaceManager *WorkspaceAddOn::workspaceManager() const {
        return m_workspaceManager;
    }
    QVariantMap WorkspaceAddOn::createDockingViewContents(int edge) const {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        ProjectWindowWorkspaceLayout::ViewSpec firstSpec;
        ProjectWindowWorkspaceLayout::ViewSpec lastSpec;
        if (edge == Qt::LeftEdge) {
            firstSpec = m_workspaceManager->currentLayout().viewSpec(ProjectWindowWorkspaceLayout::LeftTop);
            lastSpec = m_workspaceManager->currentLayout().viewSpec(ProjectWindowWorkspaceLayout::LeftBottom);
        } else if (edge == Qt::RightEdge) {
            firstSpec = m_workspaceManager->currentLayout().viewSpec(ProjectWindowWorkspaceLayout::RightTop);
            lastSpec = m_workspaceManager->currentLayout().viewSpec(ProjectWindowWorkspaceLayout::RightBottom);
        } else if (edge == Qt::TopEdge) {
            firstSpec = m_workspaceManager->currentLayout().viewSpec(ProjectWindowWorkspaceLayout::TopLeft);
            lastSpec = m_workspaceManager->currentLayout().viewSpec(ProjectWindowWorkspaceLayout::TopRight);
        } else if (edge == Qt::BottomEdge) {
            firstSpec = m_workspaceManager->currentLayout().viewSpec(ProjectWindowWorkspaceLayout::BottomLeft);
            lastSpec = m_workspaceManager->currentLayout().viewSpec(ProjectWindowWorkspaceLayout::BottomRight);
        } else {
            return {};
        }
        QVariantList result;
        QVariantList visibleIndices;
        double preferredPanelSize = edge == Qt::TopEdge || edge == Qt::BottomEdge
                                        ? qMax(firstSpec.height, lastSpec.height)
                                        : qMax(firstSpec.width, lastSpec.width);
        double splitterRatio = edge == Qt::TopEdge || edge == Qt::BottomEdge
                                   ? firstSpec.width / (firstSpec.width + lastSpec.width)
                                   : firstSpec.height / (firstSpec.height + lastSpec.height);
        auto createAction = [&](const QString &id) -> QObject * {
            auto component = iWin->actionContext()->action(id);
            if (!component) {
                return nullptr;
            }
            if (component->isError()) {
                qWarning() << component->errorString();
            }
            auto object = component->create(component->creationContext());
            if (!object) {
                return nullptr;
            }
            QQmlEngine::setObjectOwnership(object, QQmlEngine::JavaScriptOwnership);
            iWin->actionContext()->attachActionInfo(id, object);
            return object;
        };
        auto createSpec = [&](const ProjectWindowWorkspaceLayout::ViewSpec &spec) -> void {
            for (int i = 0; i < spec.panels.size(); i++) {
                const auto &[id, dock, opened, geometry, data] = spec.panels.at(i);
                auto object = createAction(id);
                if (object) {
                    // TODO: check if object has dock property (it might be an Action)
                    object->setProperty("dock", dock);
                    if (i == spec.visibleIndex || opened) {
                        visibleIndices.append(result.size());
                    }
                    if (data.isValid()) {
                        object->setProperty("panelPersistentData", data);
                    }
                    result.append(QVariantMap{
                        {"object",   QVariant::fromValue(object)},
                        {"geometry", geometry                   },
                    });
                }
            }
        };
        createSpec(firstSpec);
        if (edge != 0) {
            QQmlComponent component(PluginDatabase::qmlEngine(), "SVSCraft.UIComponents", "DockingStretch");
            auto object = component.create();
            Q_ASSERT(object);
            QQmlEngine::setObjectOwnership(object, QQmlEngine::JavaScriptOwnership);
            result.append(QVariantMap{
                {"object", QVariant::fromValue(object)},
            });
        }
        createSpec(lastSpec);

        return {
            {"objects",            result            },
            {"preferredPanelSize", preferredPanelSize},
            {"splitterRatio",      splitterRatio     },
            {"visibleIndices",     visibleIndices    },
        };
    }

    void WorkspaceAddOn::saveCustomLayoutFromJavaScript(
        const ProjectWindowWorkspaceLayout &layout_, const QString &originName,
        const QString &name) const {
        auto layout = layout_;
        layout.setName(name);
        auto customLayouts = ProjectWindowWorkspaceManager::customLayouts();
        if (originName.isEmpty()) {
            auto it = std::ranges::find_if(customLayouts,
                                           [&](const auto &o) { return o.name() == name; });
            if (it != customLayouts.end()) {
                *it = layout;
            } else {
                customLayouts.append(layout);
            }
        } else {
            customLayouts.removeIf([&](const auto &o) { return o.name() == name; });
            auto it = std::ranges::find_if(customLayouts,
                                           [&](const auto &o) { return o.name() == originName; });
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
    QVariantList WorkspaceAddOn::panelEntries() const {
        QVariantList ret;
        auto iWin = windowHandle()->cast<IProjectWindow>();
        auto a = ICore::actionRegistry()->catalog().children("core.workspacePanelWidgets") | std::views::filter([=](const QString &id) -> bool {
            return iWin->actionContext()->action(id);
        });
        std::ranges::transform(a, std::back_inserter(ret), [](const QString &id) {
            auto info = ICore::actionRegistry()->actionInfo(id);
            return QVariantMap{
                {"id", id},
                {"text", info.text()},
                {"iconSource", QUrl::fromLocalFile(ICore::actionRegistry()->actionIcon("", info.id()).filePath(QIcon::Normal, QIcon::Off))},
                {"unique", info.attributes().contains(QAK::ActionAttributeKey("uniquePanel", "http://schemas.diffscope.org/diffscope/actions/diffscope"))}
            };
        });
        return ret;
    }
}

#include "moc_workspaceaddon.cpp"
