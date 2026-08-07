#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONVIEWMODEL_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONVIEWMODEL_H

#include <QObject>

#include <SVSCraftCore/SVSCraftNamespace.h>

namespace Core {
    class NotificationMessage;
}

namespace Core::Internal {

    class NotificationManager;

    class NotificationViewModel : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString topMessageTitle READ topMessageTitle NOTIFY topMessageTitleChanged)
        Q_PROPERTY(int criticalCount READ criticalCount NOTIFY criticalCountChanged)
        Q_PROPERTY(int warningCount READ warningCount NOTIFY warningCountChanged)
    public:
        explicit NotificationViewModel(const QList<NotificationManager *> &notificationManagers, QObject *parent = nullptr);
        ~NotificationViewModel() override;

        Q_INVOKABLE QList<NotificationMessage *> messages() const;
        Q_INVOKABLE QList<NotificationMessage *> bubbleMessages() const;

        QString topMessageTitle() const;
        int criticalCount() const;
        int warningCount() const;

    Q_SIGNALS:
        void messageAdded(int index, NotificationMessage *message);
        void messageRemoved(int index, NotificationMessage *message);
        void messageAddedToBubbles(int index, NotificationMessage *message);
        void messageRemovedFromBubbles(int index, NotificationMessage *message);

        void topMessageTitleChanged(const QString &title);
        void criticalCountChanged(int count);
        void warningCountChanged(int count);
        void errorActivated();

    private:
        quint64 messageSequence(NotificationMessage *message) const;
        int insertMessage(QList<NotificationMessage *> &messages, NotificationMessage *message);
        void updateTopMessageTitleConnection();

        QList<NotificationManager *> m_notificationManagers;
        QList<NotificationMessage *> m_messages;
        QList<NotificationMessage *> m_bubbleMessages;
        QMetaObject::Connection m_topMessageTitleConnection;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_NOTIFICATIONVIEWMODEL_H
