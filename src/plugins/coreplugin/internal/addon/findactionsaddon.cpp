#include "findactionsaddon.h"

#include <QSettings>
#include <QQmlComponent>
#include <QTimer>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/actionwindowinterfacebase.h>
#include <coreplugin/internal/findactionsmodel.h>
#include <coreplugin/internal/behaviorpreference.h>

namespace Core::Internal {
    FindActionsAddOn::FindActionsAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
        m_model = new FindActionsModel(this);
        connect(BehaviorPreference::instance(), &BehaviorPreference::commandPaletteClearHistoryRequested, this, [this] {
            m_priorityActions.clear();
        });
    }
    FindActionsAddOn::~FindActionsAddOn() {
        saveSettings();
    }
    void FindActionsAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ActionWindowInterfaceBase>();
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.Core", "FindActionsAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        loadSettings();
    }
    void FindActionsAddOn::extensionsInitialized() {
        auto actionContext = windowHandle()->cast<ActionWindowInterfaceBase>()->actionContext();
        m_model->setActions(actionContext->actions());
        connect(actionContext, &QAK::QuickActionContext::actionsChanged, this, [=, this] {
            m_model->setActions(actionContext->actions());
        });
        loadSettings();
        m_model->setPriorityActions(m_priorityActions);
    }
    bool FindActionsAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    void FindActionsAddOn::findActions() {
        auto windowInterface = windowHandle()->cast<ActionWindowInterfaceBase>();
        while (m_priorityActions.size() > BehaviorPreference::commandPaletteHistoryCount()) {
            m_priorityActions.removeLast();
        }
        m_model->setPriorityActions(m_priorityActions);
        m_model->refresh(windowInterface->actionContext());
        int i = windowHandle()->cast<ActionWindowInterfaceBase>()->execQuickPick(m_model, tr("Find actions"));
        if (i == -1)
            return;
        auto actionId = m_model->index(i, 0).data().toString();
        QTimer::singleShot(0, [=] {
            windowInterface->triggerAction(actionId, windowInterface->window()->property("contentItem").value<QObject *>());
        });
        m_priorityActions.removeOne(actionId);
        m_priorityActions.prepend(actionId);
        saveSettings();
    }
    void FindActionsAddOn::loadSettings() {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        m_priorityActions = settings->value(QStringLiteral("priorityActions_") + windowHandle()->metaObject()->className()).value<QStringList>();
        settings->endGroup();
    }
    void FindActionsAddOn::saveSettings() const {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("priorityActions_") + windowHandle()->metaObject()->className(), m_priorityActions);
        settings->endGroup();
    }
}
