// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONCENTER_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONCENTER_H

#include <QHash>
#include <QObject>
#include <QStringList>

namespace Core {
    class NotificationMessage;
}

namespace Core::Internal {

    class NotificationManager;

    class NotificationCenter : public QObject {
        Q_OBJECT
    public:
        explicit NotificationCenter(QObject *parent = nullptr);
        ~NotificationCenter() override;

        static NotificationCenter *instance();

        NotificationManager *globalNotificationManager() const;

        quint64 registerMessage(NotificationMessage *message);
        quint64 messageSequence(NotificationMessage *message) const;

        bool isMessageHidden(const QString &identifier) const;
        void hideMessageIdentifier(const QString &identifier);

    public Q_SLOTS:
        void clearHiddenMessageIdentifiers();

    private:
        void loadHiddenMessageIdentifiers();
        void saveHiddenMessageIdentifiers() const;

        static NotificationCenter *m_instance;
        NotificationManager *m_globalNotificationManager{};
        QHash<NotificationMessage *, quint64> m_messageSequences;
        QStringList m_hiddenMessageIdentifiers;
        quint64 m_nextMessageSequence{};
    };

}

#endif // DIFFSCOPE_COREPLUGIN_NOTIFICATIONCENTER_H
