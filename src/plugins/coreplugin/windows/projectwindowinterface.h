#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWINTERFACE_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWINTERFACE_H

#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/actionwindowinterfacebase.h>

namespace QAK {
    class QuickActionContext;
}

class QAbstractItemModel;
class QJSValue;

namespace Core {

    class NotificationMessage;
    class ProjectTimeline;
    class EditActionsHandlerRegistry;

    class ProjectWindowInterfacePrivate;

    class CORE_EXPORT ProjectWindowInterface : public ActionWindowInterfaceBase {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(ProjectTimeline *projectTimeline READ projectTimeline CONSTANT)
        Q_PROPERTY(EditActionsHandlerRegistry *mainEditActionsHandlerRegistry READ mainEditActionsHandlerRegistry CONSTANT)
        Q_DECLARE_PRIVATE(ProjectWindowInterface)
    public:
        static ProjectWindowInterface* instance();

        ProjectTimeline *projectTimeline() const;

        EditActionsHandlerRegistry *mainEditActionsHandlerRegistry() const;

        enum NotificationBubbleMode {
            NormalBubble,
            DoNotShowBubble,
            AutoHide,
        };
        Q_ENUM(NotificationBubbleMode)
        Q_INVOKABLE void sendNotification(NotificationMessage *message, NotificationBubbleMode mode = NormalBubble);
        Q_INVOKABLE void sendNotification(SVS::SVSCraft::MessageBoxIcon icon, const QString &title, const QString &text, NotificationBubbleMode mode = NormalBubble);

    protected:
        QWindow *createWindow(QObject *parent) const override;

        explicit ProjectWindowInterface(QObject *parent = nullptr);
        explicit ProjectWindowInterface(ProjectWindowInterfacePrivate &d, QObject *parent = nullptr);
        ~ProjectWindowInterface() override;

        friend class ExecutiveInterfaceRegistry<ProjectWindowInterface>;

    private:
        QScopedPointer<ProjectWindowInterfacePrivate> d_ptr;
    };

    class CORE_EXPORT ProjectWindowInterfaceRegistry : public ExecutiveInterfaceRegistry<ProjectWindowInterface> {
    public:
        static ProjectWindowInterfaceRegistry *instance();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWINTERFACE_H
