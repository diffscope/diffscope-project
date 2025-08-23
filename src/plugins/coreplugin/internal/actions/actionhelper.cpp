#include "actionhelper.h"

#include <QQmlComponent>
#include <QtQuickTemplates2/private/qquickaction_p.h>

#include <QAKQuick/quickactioncontext.h>

namespace Core::Internal {

    bool ActionHelper::triggerAction(const QAK::QuickActionContext *actionContext, const QString &id, QObject *source) {
        if (!actionContext)
            return false;
        auto component = actionContext->action(id);
        if (!component)
            return false;
        QScopedPointer object(component->create());
        auto action = qobject_cast<QQuickAction *>(object.get());
        if (!action || !action->property("enabled").toBool())
            return false;
        action->trigger(source);
        return true;
    }

}
