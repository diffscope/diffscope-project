#include "ActionHelper.h"

#include <memory>

#include <QLoggingCategory>
#include <QQmlComponent>

#include <QtQuickTemplates2/private/qquickaction_p.h>

#include <QAKQuick/quickactioncontext.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcActionHelper, "diffscope.core.actionhelper")

    bool ActionHelper::triggerAction(QAK::QuickActionContext *actionContext, const QString &id, QObject *source) {
        qCInfo(lcActionHelper) << "Triggering action" << id;
        std::unique_ptr<QObject, QScopedPointerObjectDeleteLater<QObject>> action(createActionObject(actionContext, id, true));
        if (!action) {
            qCWarning(lcActionHelper) << "Failed to create action object for" << id;
            return false;
        }
        actionContext->attachActionInfo(id, action.get());
        if (!action->property("enabled").toBool()) {
            qCInfo(lcActionHelper) << "Action" << id << "is disabled";
            return false;
        }
        QMetaObject::invokeMethod(action.get(), "trigger", source);
        return true;
    }
    QObject *ActionHelper::createActionObject(QAK::QuickActionContext *actionContext, const QString &id, bool shouldBeQuickAction) {
        qCDebug(lcActionHelper) << "Creating action object for" << id << shouldBeQuickAction;
        if (!actionContext) {
            qCWarning(lcActionHelper) << "Action context is null";
            return nullptr;
        }
        auto component = actionContext->action(id);
        if (!component) {
            qCWarning(lcActionHelper) << "Action component for" << id << "is null";
            return nullptr;
        }
        std::unique_ptr<QObject> object(component->create(component->creationContext()));
        if (!object) {
            qCWarning(lcActionHelper) << "Failed to create action object for" << id;
            return nullptr;
        }
        actionContext->attachActionInfo(id, object.get());
        if (!shouldBeQuickAction) {
            return object.release();
        }
        if (qobject_cast<QQuickAction *>(object.get())) {
            return object.release();
        }
        qCWarning(lcActionHelper) << "Action component for" << id << "is not a QuickAction";
        return nullptr;
    }

}
