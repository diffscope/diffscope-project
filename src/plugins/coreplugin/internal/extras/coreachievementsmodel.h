#ifndef COREACHIEVEMENTSMODEL_H
#define COREACHIEVEMENTSMODEL_H

#include <QStandardItemModel>
#include <qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

namespace Core::Internal {

    class CorePlugin;

    class CoreAchievementsModel : public QStandardItemModel {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
    public:
        ~CoreAchievementsModel() override;

        static CoreAchievementsModel *instance();
        static inline CoreAchievementsModel *create(QQmlEngine *, QJSEngine *engine) {
            return instance();
        }

    Q_SIGNALS:
        void achievementCompleted(const QString &id);

    private:
        friend class CorePlugin;
        explicit CoreAchievementsModel(QObject *parent = nullptr);
    };

}


#endif //COREACHIEVEMENTSMODEL_H
