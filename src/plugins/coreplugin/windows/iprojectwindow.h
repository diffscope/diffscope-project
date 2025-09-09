#ifndef DIFFSCOPE_COREPLUGIN_IPROJECTWINDOW_H
#define DIFFSCOPE_COREPLUGIN_IPROJECTWINDOW_H

#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/iactionwindowbase.h>

namespace QAK {
    class QuickActionContext;
}

class QAbstractItemModel;
class QJSValue;

namespace Core {

    class NotificationMessage;
    class ProjectTimeline;
    class EditActionsHandlerRegistry;

    class IProjectWindowPrivate;

    class CORE_EXPORT IProjectWindow : public IActionWindowBase {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(ProjectTimeline *projectTimeline READ projectTimeline CONSTANT)
        Q_PROPERTY(EditActionsHandlerRegistry *mainEditActionsHandlerRegistry READ mainEditActionsHandlerRegistry CONSTANT)
        Q_DECLARE_PRIVATE(IProjectWindow)
    public:
        static IProjectWindow* instance();

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

        explicit IProjectWindow(QObject *parent = nullptr);
        explicit IProjectWindow(IProjectWindowPrivate &d, QObject *parent = nullptr);
        ~IProjectWindow() override;

        friend class IExecutiveRegistry<IProjectWindow>;

    private:
        QScopedPointer<IProjectWindowPrivate> d_ptr;
    };

    class CORE_EXPORT IProjectWindowRegistry : public IExecutiveRegistry<IProjectWindow> {
    public:
        static IProjectWindowRegistry *instance();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_IPROJECTWINDOW_H
