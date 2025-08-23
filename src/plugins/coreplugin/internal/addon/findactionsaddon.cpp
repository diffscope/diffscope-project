#include "findactionsaddon.h"

#include <QSettings>
#include <QQmlComponent>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/iprojectwindow.h>
#include <coreplugin/internal/findactionsmodel.h>

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
    }
    void FindActionsAddOn::extensionsInitialized() {
        auto actionContext = windowHandle()->cast<IProjectWindow>()->actionContext();
        m_model->setActions(actionContext->actions());
        connect(actionContext, &QAK::QuickActionContext::actionsChanged, this, [=, this] {
            m_model->setActions(actionContext->actions());
            m_model->refresh();
        });
        loadSettings();
        m_model->setPriorityActions(m_priorityActions);
        m_model->refresh();
    }
    bool FindActionsAddOn::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }
    void FindActionsAddOn::findActions() {
        int i = windowHandle()->cast<IProjectWindow>()->execQuickPick(m_model, 0, {}, tr("Find actions"));
        if (i == -1)
            return;
        auto actionId = m_model->index(i, 0).data().toString();
        qDebug() << actionId;
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