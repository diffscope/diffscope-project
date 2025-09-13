#include "AchievementProxyModel_p.h"

#include <uishell/USDef.h>

namespace UIShell {

    AchievementProxyModel::AchievementProxyModel(QObject *parent) 
        : QIdentityProxyModel(parent) {
    }

    AchievementProxyModel::~AchievementProxyModel() = default;

    QHash<int, QByteArray> AchievementProxyModel::roleNames() const {
        static const QHash<int, QByteArray> roles {
            {USDef::AR_IdRole, "id"},
            {USDef::AR_NameRole, "name"},
            {USDef::AR_DescriptionRole, "description"},
            {USDef::AR_IconRole, "icon"},
            {USDef::AR_IconColorRole, "iconColor"},
            {USDef::AR_HiddenRole, "hidden"},
        };
        return roles;
    }

}
