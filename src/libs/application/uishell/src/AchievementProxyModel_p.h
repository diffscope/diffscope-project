#ifndef UISHELL_ACHIEVEMENTPROXYMODEL_P_H
#define UISHELL_ACHIEVEMENTPROXYMODEL_P_H

#include <QIdentityProxyModel>
#include <qqmlintegration.h>

namespace UIShell {

    class AchievementProxyModel : public QIdentityProxyModel {
        Q_OBJECT
        QML_ELEMENT
    public:
        explicit AchievementProxyModel(QObject *parent = nullptr);
        ~AchievementProxyModel() override;

        QHash<int, QByteArray> roleNames() const override;
    };

}

#endif //UISHELL_ACHIEVEMENTPROXYMODEL_P_H
