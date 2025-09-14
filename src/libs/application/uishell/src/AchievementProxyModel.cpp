#include "AchievementProxyModel_p.h"

#include <uishell/USDef.h>

namespace UIShell {

    AchievementProxyModel::AchievementProxyModel(QObject *parent) 
        : QIdentityProxyModel(parent) {
    }

    AchievementProxyModel::~AchievementProxyModel() = default;

    enum AchievementProxyRole {
        CompletedRole = USDef::AR_HiddenRole + 1,
    };

    QHash<int, QByteArray> AchievementProxyModel::roleNames() const {
        static const QHash<int, QByteArray> roles{
            {USDef::AR_IdRole,          "id"         },
            {USDef::AR_NameRole,        "name"       },
            {USDef::AR_DescriptionRole, "description"},
            {USDef::AR_IconRole,        "icon"       },
            {USDef::AR_IconColorRole,   "iconColor"  },
            {USDef::AR_HiddenRole,      "hidden"     },
            {CompletedRole,             "completed"  },
        };
        return roles;
    }

    QVariant AchievementProxyModel::data(const QModelIndex &index, int role) const {
        if (role == CompletedRole) {
            // Get the achievement ID from the source model
            const QString id = QIdentityProxyModel::data(index, USDef::AR_IdRole).toString();
            return m_completedAchievements.contains(id);
        }
        
        // For all other roles, delegate to the parent implementation
        return QIdentityProxyModel::data(index, role);
    }

    void AchievementProxyModel::setSourceModel(QAbstractItemModel *sourceModel) {
        // Clear existing mapping
        m_idToRowMap.clear();

        // Call parent implementation
        QIdentityProxyModel::setSourceModel(sourceModel);

        // Build ID to row mapping if source model is valid
        if (sourceModel) {
            const int rowCount = sourceModel->rowCount();
            for (int row = 0; row < rowCount; ++row) {
                const QModelIndex idx = sourceModel->index(row, 0);
                const QString id = sourceModel->data(idx, USDef::AR_IdRole).toString();
                if (!id.isEmpty()) {
                    m_idToRowMap.insert(id, row);
                }
            }
        }
    }

    QModelIndex AchievementProxyModel::indexForId(const QString &id) const {
        auto it = m_idToRowMap.find(id);
        if (it != m_idToRowMap.end()) {
            return index(it.value(), 0);
        }
        return QModelIndex(); // Return invalid index if not found
    }

    bool AchievementProxyModel::handleAchievementCompleted(const QString &id, bool completed) {
        bool changed = false;
        
        if (completed) {
            // Add to completed achievements if not already present
            if (!m_completedAchievements.contains(id)) {
                m_completedAchievements.insert(id);
                changed = true;
            }
        } else {
            // Remove from completed achievements if present
            if (m_completedAchievements.contains(id)) {
                m_completedAchievements.remove(id);
                changed = true;
            }
        }
        
        if (changed) {
            // Use cached mapping to find the model index directly
            const QModelIndex idx = indexForId(id);
            if (idx.isValid()) {
                emit dataChanged(idx, idx, {CompletedRole});
            }
        }
        return changed;
    }

}
