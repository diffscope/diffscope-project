#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONMANAGER_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONMANAGER_H

#include <QObject>

#include <coreplugin/NotificationMessage.h>
#include <coreplugin/ProjectWindowInterface.h>

class QAbstractItemModel;

namespace Core::Internal {

    class NotificationManager : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString topMessageTitle READ topMessageTitle NOTIFY topMessageTitleChanged)
    public:
        explicit NotificationManager(ProjectWindowInterface *parent = nullptr);
        ~NotificationManager() override;

        static NotificationManager *of(ProjectWindowInterface *windowHandle);

        void addMessage(NotificationMessage *message, ProjectWindowInterface::NotificationBubbleMode mode);

        Q_INVOKABLE QList<NotificationMessage *> messages() const;
        Q_INVOKABLE QList<NotificationMessage *> bubbleMessages() const;

        QString topMessageTitle() const;

    Q_SIGNALS:
        void messageAdded(int index, NotificationMessage *message);
        void messageRemoved(int index, NotificationMessage *message);

        void messageAddedToBubbles(int index, NotificationMessage *message);
        void messageRemovedFromBubbles(int index, NotificationMessage *message);

        void topMessageTitleChanged(const QString &title);

    private:
        void updateTopMessageTitleConnection();

        QList<NotificationMessage *> m_messages;
        QList<NotificationMessage *> m_bubbleMessages;
        QMetaObject::Connection m_topMessageTitleConnection;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONMANAGER_H
