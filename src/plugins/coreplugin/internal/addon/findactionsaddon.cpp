#include "findactionsaddon.h"

#include <QSettings>
#include <QQmlComponent>
#include <QTimer>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/iprojectwindow.h>
#include <coreplugin/internal/findactionsmodel.h>
#include <coreplugin/internal/behaviorpreference.h>

namespace Core::Internal {
    FindActionsAddOn::FindActionsAddOn(QObject *parent) : IWindowAddOn(parent) {
        m_model = new FindActionsModel(this);
    }
    FindActionsAddOn::~FindActionsAddOn() {
        saveSettings();
    }
    void FindActionsAddOn::initialize() {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "FindActionsAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", iWin->actionContext());
        loadSettings();
    }
    void FindActionsAddOn::extensionsInitialized() {
        auto actionContext = windowHandle()->cast<IProjectWindow>()->actionContext();
        m_model->setActions(actionContext->actions());
        connect(actionContext, &QAK::QuickActionContext::actionsChanged, this, [=, this] {
            m_model->setActions(actionContext->actions());
        });
        loadSettings();
        m_model->setPriorityActions(m_priorityActions);
    }
    bool FindActionsAddOn::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }
    void FindActionsAddOn::findActions() {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        while (m_priorityActions.size() > BehaviorPreference::commandPaletteHistoryCount()) {
            m_priorityActions.removeLast();
        }
        m_model->setPriorityActions(m_priorityActions);
        m_model->refresh(iWin->actionContext());
        int i = windowHandle()->cast<IProjectWindow>()->execQuickPick(m_model, 0, {}, tr("Find actions"));
        if (i == -1)
            return;
        auto actionId = m_model->index(i, 0).data().toString();
        QTimer::singleShot(0, [=] {
            iWin->triggerAction(actionId, iWin->window()->property("contentItem").value<QObject *>());
        });
        m_priorityActions.removeOne(actionId);
        m_priorityActions.prepend(actionId);
        saveSettings();
    }
    void FindActionsAddOn::loadSettings() {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        m_priorityActions = settings->value("priorityActions").value<QStringList>();
        settings->endGroup();
    }
    void FindActionsAddOn::saveSettings() const {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("priorityActions", m_priorityActions);
        settings->endGroup();
    }
}