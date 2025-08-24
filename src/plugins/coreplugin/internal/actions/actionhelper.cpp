#include "actionhelper.h"

#include <memory>

#include <QQmlComponent>
#include <QtQuickTemplates2/private/qquickaction_p.h>

#include <QAKQuick/quickactioncontext.h>

namespace Core::Internal {

    bool ActionHelper::triggerAction(const QAK::QuickActionContext *actionContext,
                                     const QString &id, QObject *source) {
        std::unique_ptr<QObject, QScopedPointerObjectDeleteLater<QObject>> action(createActionObject(actionContext, id, true));
        if (!action || !action->property("enabled").toBool())
            return false;
        QMetaObject::invokeMethod(action.get(), "trigger", source);
        return true;
    }
    QObject *ActionHelper::createActionObject(const QAK::QuickActionContext *actionContext, const QString &id, bool shouldBeQuickAction) {
        if (!actionContext)
            return nullptr;
        auto component = actionContext->action(id);
        if (!component)
            return nullptr;
        std::unique_ptr<QObject> object(component->create(component->creationContext()));
        if (!shouldBeQuickAction) {
            return object.release();
        }
        if (qobject_cast<QQuickAction *>(object.get())) {
            return object.release();
        }
        return nullptr;
    }

}
