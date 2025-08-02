#include "projectwindowdata.h"

#include <QQmlComponent>

#include <coreplugin/icore.h>
#include <coreplugin/internal/projectwindowworkspacemanager.h>

namespace Core::Internal {
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
        QObjectList result;
        QVariantList visibleIndices;
        double preferredPanelSize = edge == Qt::TopEdge || edge == Qt::BottomEdge ? qMax(firstSpec.height, lastSpec.height) : qMax(firstSpec.width, lastSpec.width); // TODO: dual pane
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
            actionContext()->attachActionInfo(id, object);
            return object;
        };
        auto createSpec = [&](const ProjectWindowWorkspaceLayout::ViewSpec &spec) -> void {
            for (int i = 0; i < spec.panels.size(); i++) {
                const auto &[id, dock] = spec.panels.at(i);
                auto object = createAction(id);
                if (object) {
                    // TODO: check if object has dock property (it might be an Action)
                    object->setProperty("dock", dock);
                    if (i == spec.visibleIndex) {
                        visibleIndices.append(result.size());
                    }
                    result.append(object);
                }
            }
        };
        createSpec(firstSpec);
        if (edge != 0) {
            QQmlComponent component(ICore::qmlEngine(), "SVSCraft.UIComponents", "DockingStretch");
            auto object = component.create();
            Q_ASSERT(object);
            result.append(object);
        }
        createSpec(lastSpec);

        return {
            {"objects", QVariant::fromValue(result)},
            {"preferredPanelSize", preferredPanelSize},
            {"visibleIndices", visibleIndices},
        };
    }
}

#include "moc_projectwindowdata.cpp"