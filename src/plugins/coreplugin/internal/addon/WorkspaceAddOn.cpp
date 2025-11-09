#include "WorkspaceAddOn.h"

#include <algorithm>
#include <ranges>

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QStandardItemModel>

#include <CoreApi/runtimeinterface.h>

#include <QAKCore/actionregistry.h>
#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/ProjectWindowWorkspaceLayout.h>
#include <coreplugin/internal/ProjectWindowWorkspaceManager.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcWorkspaceAddOn, "diffscope.core.workspaceaddon")

    WorkspaceAddOn::WorkspaceAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
        m_workspaceManager = new ProjectWindowWorkspaceManager(this);
        m_workspaceManager->load();
    }
    WorkspaceAddOn::~WorkspaceAddOn() {
        m_workspaceManager->save();
    }
    void WorkspaceAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "WorkspaceAddOnHelper");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            m_helper = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}
            });
            m_helper->setParent(windowInterface->window());
        }
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "WorkspaceAddOnActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}, {"helper", QVariant::fromValue(m_helper.get())}});
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());

            windowInterface->actionContext()->addAction("org.diffscope.core.panel.properties", new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", "PropertiesPanel", this));
            windowInterface->actionContext()->addAction("org.diffscope.core.panel.plugins", new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", "PluginsPanel", this));
            windowInterface->actionContext()->addAction("org.diffscope.core.panel.tips", new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", "TipsPanel", this));
            windowInterface->actionContext()->addAction("org.diffscope.core.panel.arrangement", new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ArrangementPanel", this));
            windowInterface->actionContext()->addAction("org.diffscope.core.panel.mixer", new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", "MixerPanel", this));
            windowInterface->actionContext()->addAction("org.diffscope.core.panel.pianoRoll", new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", "PianoRollPanel", this));
        }
    }
    void WorkspaceAddOn::extensionsInitialized() {
        windowHandle()->window()->installEventFilter(this);
    }
    bool WorkspaceAddOn::delayedInitialize() {
        emit panelEntriesChanged();
        return WindowInterfaceAddOn::delayedInitialize();
    }
    ProjectWindowWorkspaceManager *WorkspaceAddOn::workspaceManager() const {
        return m_workspaceManager;
    }
    QVariantMap WorkspaceAddOn::createDockingViewContents(int edge) const {
        qCDebug(lcWorkspaceAddOn) << "Creating docking view contents of edge" << edge;
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
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
        qCDebug(lcWorkspaceAddOn).noquote().nospace() << "First spec "
                                                      << "("
                                                      << "width=" << firstSpec.width << ","
                                                      << "height=" << firstSpec.height << ","
                                                      << "visibleIndex=" << firstSpec.visibleIndex << ")";
        qCDebug(lcWorkspaceAddOn).noquote().nospace() << "Last spec "
                                                      << "("
                                                      << "width=" << lastSpec.width << ","
                                                      << "height=" << lastSpec.height << ","
                                                      << "visibleIndex=" << lastSpec.visibleIndex << ")";
        QVariantList result;
        QVariantList visibleIndices;
        double preferredPanelSize = edge == Qt::TopEdge || edge == Qt::BottomEdge
                                        ? qMax(firstSpec.height, lastSpec.height)
                                        : qMax(firstSpec.width, lastSpec.width);
        qCDebug(lcWorkspaceAddOn) << "Calculated preferred panel size" << preferredPanelSize;
        double splitterRatio = edge == Qt::TopEdge || edge == Qt::BottomEdge
                                   ? firstSpec.width / (firstSpec.width + lastSpec.width)
                                   : firstSpec.height / (firstSpec.height + lastSpec.height);
        qCDebug(lcWorkspaceAddOn) << "Calculated splitter ratio" << splitterRatio;
        auto createAction = [&](const QString &id) -> QObject * {
            auto component = windowInterface->actionContext()->action(id);
            if (!component) {
                qCWarning(lcWorkspaceAddOn) << "Action" << id << "not found";
                return nullptr;
            }
            if (component->isError()) {
                qCWarning(lcWorkspaceAddOn) << "Action" << id << "component has error";
                qWarning() << component->errorString();
                return nullptr;
            }
            auto object = component->create(component->creationContext());
            if (!object) {
                qCWarning(lcWorkspaceAddOn) << "Action" << id << "component cannot be created";
                qWarning() << component->errorString();
                return nullptr;
            }
            QQmlEngine::setObjectOwnership(object, QQmlEngine::JavaScriptOwnership);
            windowInterface->actionContext()->attachActionInfo(id, object);
            return object;
        };
        auto createSpec = [&](const ProjectWindowWorkspaceLayout::ViewSpec &spec) -> void {
            for (int i = 0; i < spec.panels.size(); i++) {
                const auto &[id, dock, opened, geometry, data] = spec.panels.at(i);
                qCDebug(lcWorkspaceAddOn) << "Creating panel" << id << dock << opened << geometry;
                auto object = createAction(id);
                if (object) {
                    qCDebug(lcWorkspaceAddOn) << "Created object" << object;
                    // TODO: check if object has dock property (it might be an Action)
                    object->setProperty("dock", dock);
                    if (i == spec.visibleIndex || opened) {
                        visibleIndices.append(result.size());
                    }
                    if (data.isValid()) {
                        object->setProperty("panelPersistentData", data);
                    }
                    result.append(QVariantMap{
                        {"object", QVariant::fromValue(object)},
                        {"geometry", geometry},
                    });
                }
            }
        };
        qCDebug(lcWorkspaceAddOn) << "Creating first spec";
        createSpec(firstSpec);
        if (edge != 0) {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "DockingStretch");
            auto object = component.create();
            Q_ASSERT(object);
            QQmlEngine::setObjectOwnership(object, QQmlEngine::JavaScriptOwnership);
            result.append(QVariantMap{
                {"object", QVariant::fromValue(object)},
            });
        }
        qCDebug(lcWorkspaceAddOn) << "Creating last spec";
        createSpec(lastSpec);

        return {
            {"objects", result},
            {"preferredPanelSize", preferredPanelSize},
            {"splitterRatio", splitterRatio},
            {"visibleIndices", visibleIndices},
        };
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
                QMetaObject::invokeMethod(m_helper, "promptSaveLayout", QVariant::fromValue(m_workspaceManager->currentLayout()), QVariant::fromValue(layout.name()));
                break;
            }
            case 2: {
                QMetaObject::invokeMethod(m_helper, "promptDeleteLayout", QVariant::fromValue(layout.name()));
                break;
            }
        }
    }
    QVariantList WorkspaceAddOn::panelEntries() const {
        QVariantList ret;
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto a = CoreInterface::actionRegistry()->catalog().children("org.diffscope.core.workspacePanelWidgets") | std::views::filter([=](const QString &id) -> bool { return windowInterface->actionContext()->action(id); });
        std::ranges::transform(a, std::back_inserter(ret), [](const QString &id) {
            auto info = CoreInterface::actionRegistry()->actionInfo(id);
            auto actionIcon = CoreInterface::actionRegistry()->actionIcon("", info.id());
            return QVariantMap{
                {"id", id},
                {"text", info.text()},
                {"iconSource", QUrl::fromLocalFile(actionIcon.filePath())},
                {"iconColor", QColor::fromString(actionIcon.currentColor())},
                {"unique", info.attributes().contains(QAK::ActionAttributeKey("uniquePanel", "http://schemas.diffscope.org/diffscope/actions/diffscope"))}
            };
        });
        return ret;
    }
    bool WorkspaceAddOn::eventFilter(QObject *watched, QEvent *event) {
        if (event->type() == QEvent::Expose && windowHandle()->window()->isExposed()) {
            Q_EMIT windowExposed();
            windowHandle()->window()->removeEventFilter(this);
        }
        return WindowInterfaceAddOn::eventFilter(watched, event);
    }
}

#include "moc_WorkspaceAddOn.cpp"
