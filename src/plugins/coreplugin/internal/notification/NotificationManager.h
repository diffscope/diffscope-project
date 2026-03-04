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
        Q_PROPERTY(int criticalCount READ criticalCount NOTIFY criticalCountChanged)
        Q_PROPERTY(int warningCount READ warningCount NOTIFY warningCountChanged)
    public:
        explicit NotificationManager(ProjectWindowInterface *parent = nullptr);
        ~NotificationManager() override;

        static NotificationManager *of(ProjectWindowInterface *windowHandle);

        void addMessage(NotificationMessage *message, ProjectWindowInterface::NotificationBubbleMode mode);

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
        void updateTopMessageTitleConnection();
        void updateIconCount(SVS::SVSCraft::MessageBoxIcon oldIcon, SVS::SVSCraft::MessageBoxIcon newIcon);
        void disconnectMessageSignals(NotificationMessage *message);
        void loadHiddenMessageIdentifiers();
        void saveHiddenMessageIdentifiers();
        bool isMessageHidden(const QString &identifier) const;
        void clearHiddenMessageIdentifiers();

        QList<NotificationMessage *> m_messages;
        QList<NotificationMessage *> m_bubbleMessages;
        QMetaObject::Connection m_topMessageTitleConnection;
        QStringList m_hiddenMessageIdentifiers;
        int m_criticalCount = 0;
        int m_warningCount = 0;
        QHash<NotificationMessage *, SVS::SVSCraft::MessageBoxIcon> m_messageIcons;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONMANAGER_H
