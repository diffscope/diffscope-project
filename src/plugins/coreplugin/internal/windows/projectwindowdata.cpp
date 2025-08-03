#include "projectwindowdata.h"

#include <QQmlComponent>
#include <QQmlEngine>

#include <SVSCraftQuick/MessageBox.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/internal/projectwindowworkspacemanager.h>

namespace Core::Internal {

    static const char viewVisibilitySettingCatalogC[] = "ProjectWindowData/viewVisibility";

    ProjectWindowData::ProjectWindowData(QObject *parent) : QObject(parent) {
        m_actionContext = new QAK::QuickActionContext(this);
        m_actionContext->setMenuComponent(new QQmlComponent(ICore::qmlEngine(), "SVSCraft.UIComponents", "Menu", this));
        m_actionContext->setSeparatorComponent(new QQmlComponent(ICore::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", this));
        m_actionContext->setStretchComponent(new QQmlComponent(ICore::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", this));

        m_workspaceManager = new ProjectWindowWorkspaceManager(this);
        m_workspaceManager->load();
    }
    ProjectWindowData::~ProjectWindowData() {
        m_workspaceManager->save();
    }
    QAK::QuickActionContext *ProjectWindowData::actionContext() const {
        return m_actionContext;
    }
    ProjectWindowWorkspaceManager *ProjectWindowData::workspaceManager() const {
        return m_workspaceManager;
    }
    ProjectWindowData::PanelPosition ProjectWindowData::activePanel() const {
        return m_activePanel;
    }
    void ProjectWindowData::setActivePanel(PanelPosition panel) {
        if (m_activePanel == panel) {
            return;
        }
        m_activePanel = panel;
        emit activePanelChanged();
    }
    QObject *ProjectWindowData::activeUndockedPane() const {
        return m_activeUndockedPane;
    }
    void ProjectWindowData::setActiveUndockedPane(QObject *undockedPane) {
        if (m_activeUndockedPane == undockedPane) {
            return;
        }
        m_activeUndockedPane = undockedPane;
        emit activeUndockedPaneChanged();
    }
    QObject *ProjectWindowData::activeDockingPane() const {
        return m_activeDockingPane;
    }
    void ProjectWindowData::setActiveDockingPane(QObject *dockingPane) {
        if (m_activeDockingPane == dockingPane) {
            return;
        }
        m_activeDockingPane = dockingPane;
        emit activeDockingPaneChanged();
    }
    QObjectList ProjectWindowData::dockingPanes() const {
        return m_dockingPanes;
    }
    void ProjectWindowData::setDockingPanes(const QObjectList &dockingPanes) {
        m_dockingPanes = dockingPanes;
        emit dockingPanesChanged();
    }
    QObjectList ProjectWindowData::floatingPanes() const {
        return m_floatingPanes;
    }
    void ProjectWindowData::setFloatingPanes(const QObjectList &floatingPanes) {
        m_floatingPanes = floatingPanes;
        emit floatingPanesChanged();
    }
    QVariantMap ProjectWindowData::createDockingViewContents(int edge) const {
        ProjectWindowWorkspaceLayout::ViewSpec firstSpec;
        ProjectWindowWorkspaceLayout::ViewSpec lastSpec;
        if (edge == 0) {
            firstSpec = m_workspaceManager->currentLayout().viewSpec(Windowed);
        } else if (edge == Qt::LeftEdge) {
            firstSpec = m_workspaceManager->currentLayout().viewSpec(LeftTop);
            lastSpec = m_workspaceManager->currentLayout().viewSpec(LeftBottom);
        } else if (edge == Qt::RightEdge) {
            firstSpec = m_workspaceManager->currentLayout().viewSpec(RightTop);
            lastSpec = m_workspaceManager->currentLayout().viewSpec(RightBottom);
        } else if (edge == Qt::TopEdge) {
            firstSpec = m_workspaceManager->currentLayout().viewSpec(TopLeft);
            lastSpec = m_workspaceManager->currentLayout().viewSpec(TopRight);
        } else if (edge == Qt::BottomEdge) {
            firstSpec = m_workspaceManager->currentLayout().viewSpec(BottomLeft);
            lastSpec = m_workspaceManager->currentLayout().viewSpec(BottomRight);
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
            auto component = m_actionContext->action(id);
            if (!component) {
                return nullptr;
            }
            if (component->isError()) {
                qWarning() << component->errorString();
            }
            auto object = component->create();
            if (!object) {
                return nullptr;
            }
            QQmlEngine::setObjectOwnership(object, QQmlEngine::JavaScriptOwnership);
            actionContext()->attachActionInfo(id, object);
            return object;
        };
        auto createSpec = [&](const ProjectWindowWorkspaceLayout::ViewSpec &spec) -> void {
            for (int i = 0; i < spec.panels.size(); i++) {
                const auto &[id, dock, opened, geometry] = spec.panels.at(i);
                auto object = createAction(id);
                if (object) {
                    // TODO: check if object has dock property (it might be an Action)
                    object->setProperty("dock", dock);
                    if (i == spec.visibleIndex || opened) {
                        visibleIndices.append(result.size());
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
            QQmlComponent component(ICore::qmlEngine(), "SVSCraft.UIComponents", "DockingStretch");
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
    void ProjectWindowData::toggleVisibility(ViewVisibilityOption option, bool visible,
                                             QObject *action) const {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(viewVisibilitySettingCatalogC);
        if (option == MenuBar) {
            auto menuBar = window()->property("menuBar").value<QObject *>();
            if (!visible) {
                if (SVS::SVSCraft::No ==
                    SVS::MessageBox::warning(
                        ICore::qmlEngine(), window(), tr("Please take attention"),
                        tr("After hiding the menu bar, it can be difficult to show it again. Make "
                           "sure you know how to do this.\n\nContinue?"),
                        SVS::SVSCraft::Yes | SVS::SVSCraft::No, SVS::SVSCraft::No)) {
                    action->setProperty("checked", true);
                    goto end;
                }
            } else {
                menuBar->setProperty("visible", visible);
            }
        } else if (option == ToolBar) {
            auto toolBar = window()->property("toolBar").value<QObject *>();
            toolBar->setProperty("visible", visible);
        } else if (option == LeftSideBar) {
            auto leftDockingView = window()->property("leftDockingView").value<QObject *>();
            leftDockingView->setProperty("barSize", visible ? 32 : 0);
        } else if (option == RightSideBar) {
            auto rightDockingView = window()->property("rightDockingView").value<QObject *>();
            rightDockingView->setProperty("barSize", visible ? 32 : 0);
        } else if (option == TopSideBar) {
            auto topDockingView = window()->property("topDockingView").value<QObject *>();
            topDockingView->setProperty("barSize", visible ? 32 : 0);
        } else if (option == BottomSideBar) {
            auto bottomDockingView = window()->property("bottomDockingView").value<QObject *>();
            bottomDockingView->setProperty("barSize", visible ? 32 : 0);
        } else if (option == StatusBar) {
            auto statusBar = window()->property("statusBar").value<QObject *>();
            statusBar->setProperty("visible", visible);
        }
        settings->setValue(QString::number(option), !visible);
    end:
        settings->endGroup();
    }
    void ProjectWindowData::loadVisibility(QObject *window) {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(viewVisibilitySettingCatalogC);

        auto menuBarVisible = !settings->value(QString::number(MenuBar)).value<bool>();
        auto menuBar = window->property("menuBar").value<QObject *>();
        menuBar->setProperty("visible", menuBarVisible);

        auto toolBarVisible = !settings->value(QString::number(ToolBar)).value<bool>();
        auto toolBar = window->property("toolBar").value<QObject *>();
        toolBar->setProperty("visible", toolBarVisible);

        auto leftSideBarVisible = !settings->value(QString::number(LeftSideBar)).value<bool>();
        auto leftDockingView = window->property("leftDockingView").value<QObject *>();
        leftDockingView->setProperty("barSize", leftSideBarVisible ? 32 : 0);

        auto rightSideBarVisible = !settings->value(QString::number(RightSideBar)).value<bool>();
        auto rightDockingView = window->property("rightDockingView").value<QObject *>();
        rightDockingView->setProperty("barSize", rightSideBarVisible ? 32 : 0);

        auto topSideBarVisible = !settings->value(QString::number(TopSideBar)).value<bool>();
        auto topDockingView = window->property("topDockingView").value<QObject *>();
        topDockingView->setProperty("barSize", topSideBarVisible ? 32 : 0);

        auto bottomSideBarVisible = !settings->value(QString::number(BottomSideBar)).value<bool>();
        auto bottomDockingView = window->property("bottomDockingView").value<QObject *>();
        bottomDockingView->setProperty("barSize", bottomSideBarVisible ? 32 : 0);

        auto statusBarVisible = !settings->value(QString::number(StatusBar)).value<bool>();
        auto statusBar = window->property("statusBar").value<QObject *>();
        statusBar->setProperty("visible", statusBarVisible);

        settings->endGroup();
    }
    void ProjectWindowData::saveCustomLayoutFromJavaScript(const QString &name) const {
        ProjectWindowWorkspaceLayout layout = m_workspaceManager->currentLayout();
        layout.setName(name);
        auto customLayouts = ProjectWindowWorkspaceManager::customLayouts();
        bool replaced = false;
        for (int i = 0; i < customLayouts.size(); ++i) {
            if (customLayouts[i].name() == name) {
                customLayouts[i] = layout;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            customLayouts.append(layout);
        }
        ProjectWindowWorkspaceManager::setCustomLayouts(customLayouts);
        m_workspaceManager->save();
    }
}

#include "moc_projectwindowdata.cpp"