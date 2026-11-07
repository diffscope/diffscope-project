// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "NotificationViewModel.h"

#include <QAbstractItemModel>

#include <notifications/NotificationCenter.h>
#include <notifications/NotificationMessage.h>
#include <notifications/NotificationModel.h>

namespace Core::Internal {

    NotificationViewModel::NotificationViewModel(Notifications::NotificationCenter *center, QObject *scope, QObject *parent)
        : QObject(parent), m_notifications(center->notificationsForScope(scope)) {
        connect(m_notifications, &QAbstractItemModel::rowsInserted, this, &NotificationViewModel::updateSummary);
        connect(m_notifications, &QAbstractItemModel::rowsRemoved, this, &NotificationViewModel::updateSummary);
        connect(m_notifications, &QAbstractItemModel::rowsMoved, this, &NotificationViewModel::updateSummary);
        connect(m_notifications, &QAbstractItemModel::dataChanged, this, &NotificationViewModel::updateSummary);
        connect(m_notifications, &QAbstractItemModel::modelReset, this, &NotificationViewModel::updateSummary);
        connect(m_notifications, &QAbstractItemModel::layoutChanged, this, &NotificationViewModel::updateSummary);
        connect(center, &Notifications::NotificationCenter::errorActivated, this, [this, scope](QObject *messageScope) {
            if (!messageScope || messageScope == scope) {
                Q_EMIT errorActivated();
            }
        });
        updateSummary();
    }

    NotificationViewModel::~NotificationViewModel() = default;

    int NotificationViewModel::count() const {
        return m_count;
    }

    QString NotificationViewModel::topMessageTitle() const {
        return m_topMessageTitle;
    }

    int NotificationViewModel::criticalCount() const {
        return m_criticalCount;
    }

    int NotificationViewModel::warningCount() const {
        return m_warningCount;
    }

    SVS::SVSCraft::ControlType NotificationViewModel::badgeType() const {
        return m_badgeType;
    }

    void NotificationViewModel::updateSummary() {
        const auto count = m_notifications->rowCount();
        QString topMessageTitle;
        int criticalCount = 0;
        int warningCount = 0;
        bool hasAccent = false;
        for (int row = 0; row < count; ++row) {
            auto *message = m_notifications->messageAt(row);
            if (!message) {
                continue;
            }
            if (row == 0) {
                topMessageTitle = message->title();
            }
            switch (message->icon()) {
                case SVS::SVSCraft::Critical:
                    ++criticalCount;
                    break;
                case SVS::SVSCraft::Warning:
                    ++warningCount;
                    break;
                case SVS::SVSCraft::Success:
                case SVS::SVSCraft::Tip:
                    hasAccent = true;
                    break;
                default:
                    break;
            }
        }
        const auto badgeType = criticalCount ? SVS::SVSCraft::CT_Error : warningCount ? SVS::SVSCraft::CT_Warning : hasAccent ? SVS::SVSCraft::CT_Accent : SVS::SVSCraft::CT_Normal;
        if (m_count == count && m_topMessageTitle == topMessageTitle && m_criticalCount == criticalCount && m_warningCount == warningCount && m_badgeType == badgeType) {
            return;
        }
        m_count = count;
        m_topMessageTitle = topMessageTitle;
        m_criticalCount = criticalCount;
        m_warningCount = warningCount;
        m_badgeType = badgeType;
        Q_EMIT summaryChanged();
    }

}

#include "moc_NotificationViewModel.cpp"
