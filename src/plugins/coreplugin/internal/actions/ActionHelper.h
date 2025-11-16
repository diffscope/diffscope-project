#ifndef DIFFSCOPE_UTILS_ACTIONHELPER_H
#define DIFFSCOPE_UTILS_ACTIONHELPER_H

#include <QObject>
#include <QString>

namespace QAK {
    class QuickActionContext;
}

namespace Core::Internal {

    class ActionHelper {
    public:
        static bool triggerAction(QAK::QuickActionContext *actionContext, const QString &id, QObject *source = nullptr);
        static QObject *createActionObject(QAK::QuickActionContext *actionContext, const QString &id, bool shouldBeQuickAction = true);
    };

}

#endif // DIFFSCOPE_UTILS_ACTIONHELPER_H
