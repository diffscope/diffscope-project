#include "AchievementProxyModel_p.h"

#include <uishell/USDef.h>

namespace UIShell {

    AchievementProxyModel::AchievementProxyModel(QObject *parent) 
        : QIdentityProxyModel(parent) {
    }

    AchievementProxyModel::~AchievementProxyModel() = default;

    QHash<int, QByteArray> AchievementProxyModel::roleNames() const {
        static const QHash<int, QByteArray> roles {
            {USDef::AR_NameRole, "name"},
            {USDef::AR_DescriptionRole, "description"},
            {USDef::AR_IconRole, "icon"},
            {USDef::AR_IconColorRole, "iconColor"},
            {USDef::AR_ConditionRole, "condition"},
            {USDef::AR_HiddenRole, "hidden"},
            {USDef::AR_CompletedRole, "completed"},
        };
        return roles;
    }

}
