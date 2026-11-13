// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONVIEWMODEL_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONVIEWMODEL_H

#include <QObject>
#include <QString>

#include <SVSCraftCore/SVSCraftNamespace.h>

namespace Notifications {
    class NotificationCenter;
    class NotificationModel;
}

namespace Core::Internal {

    class NotificationViewModel : public QObject {
        Q_OBJECT
        Q_PROPERTY(int count READ count NOTIFY summaryChanged)
        Q_PROPERTY(QString topMessageTitle READ topMessageTitle NOTIFY summaryChanged)
        Q_PROPERTY(int criticalCount READ criticalCount NOTIFY summaryChanged)
        Q_PROPERTY(int warningCount READ warningCount NOTIFY summaryChanged)
        Q_PROPERTY(SVS::SVSCraft::ControlType badgeType READ badgeType NOTIFY summaryChanged)
    public:
        explicit NotificationViewModel(Notifications::NotificationCenter *center, QObject *scope, QObject *parent = nullptr);
        ~NotificationViewModel() override;

        int count() const;
        QString topMessageTitle() const;
        int criticalCount() const;
        int warningCount() const;
        SVS::SVSCraft::ControlType badgeType() const;

    Q_SIGNALS:
        void summaryChanged();

    private:
        void updateSummary();

        Notifications::NotificationModel *m_notifications{};
        int m_count{};
        QString m_topMessageTitle;
        int m_criticalCount{};
        int m_warningCount{};
        SVS::SVSCraft::ControlType m_badgeType{SVS::SVSCraft::CT_Normal};
    };

}

#endif // DIFFSCOPE_COREPLUGIN_NOTIFICATIONVIEWMODEL_H
