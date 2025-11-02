#ifndef UISHELL_ACHIEVEMENTPROXYMODEL_P_H
#define UISHELL_ACHIEVEMENTPROXYMODEL_P_H

#include <qqmlintegration.h>

#include <QIdentityProxyModel>

namespace UIShell {

    class AchievementProxyModel : public QIdentityProxyModel {
        Q_OBJECT
        QML_ELEMENT
    public:
        explicit AchievementProxyModel(QObject *parent = nullptr);
        ~AchievementProxyModel() override;

        QHash<int, QByteArray> roleNames() const override;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        void setSourceModel(QAbstractItemModel *sourceModel) override;

        Q_INVOKABLE QModelIndex indexForId(const QString &id) const;

        Q_INVOKABLE bool handleAchievementCompleted(const QString &id, bool completed);

    private:
        QSet<QString> m_completedAchievements;
        QHash<QString, int> m_idToRowMap;
    };

}

#endif //UISHELL_ACHIEVEMENTPROXYMODEL_P_H
