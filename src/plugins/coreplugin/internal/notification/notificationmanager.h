#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONMANAGER_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONMANAGER_H

#include <QObject>

#include <coreplugin/iprojectwindow.h>
#include <coreplugin/notificationmessage.h>

class QAbstractItemModel;

namespace Core::Internal {

    class NotificationManager : public QObject {
        Q_OBJECT
    public:
        explicit NotificationManager(IProjectWindow *parent = nullptr);
        ~NotificationManager() override;

        static NotificationManager *of(IProjectWindow *windowHandle);

        void addMessage(NotificationMessage *message, IProjectWindow::NotificationBubbleMode mode);

        Q_INVOKABLE QList<NotificationMessage *> messages() const;
        Q_INVOKABLE QList<NotificationMessage *> bubbleMessages() const;

    Q_SIGNALS:
        void messageAdded(int index, NotificationMessage *message);
        void messageRemoved(int index, NotificationMessage *message);

        void messageAddedToBubbles(int index, NotificationMessage *message);
        void messageRemovedFromBubbles(int index, NotificationMessage *message);

    private:
        QList<NotificationMessage *> m_messages;
        QList<NotificationMessage *> m_bubbleMessages;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONMANAGER_H
