#include "RecentFilesProxyModel_p.h"

#include <uishell/USDef.h>

namespace UIShell {

    RecentFilesProxyModel::RecentFilesProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
        connect(this, &QAbstractItemModel::modelReset, this, &RecentFilesProxyModel::countChanged);
        connect(this, &QAbstractItemModel::rowsInserted, this, &RecentFilesProxyModel::countChanged);
        connect(this, &QAbstractItemModel::rowsRemoved, this, &RecentFilesProxyModel::countChanged);
    }
    RecentFilesProxyModel::~RecentFilesProxyModel() = default;
    QHash<int, QByteArray> RecentFilesProxyModel::roleNames() const {
        static const QHash<int, QByteArray> m{
            {USDef::RF_NameRole, "name"},
            {USDef::RF_PathRole, "path"},
            {USDef::RF_LastModifiedTextRole, "lastModifiedText"},
            {USDef::RF_ThumbnailRole, "thumbnail"},
            {USDef::RF_IconRole, "icon"},
            {USDef::RF_ColorizeRole, "colorize"},
        };
        return m;
    }

}
