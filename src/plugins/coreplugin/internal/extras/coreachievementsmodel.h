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

        enum Achievement {
            Achievement_DiffScope,
            Achievement_NewProject,
            Achievement_FindActions,
            Achievement_Help,
            Achievement_ContextHelpTip,
            Achievement_DisableCustomTitleBar,
            Achievement_DisableAnimation,
            Achievement_CommandLineSettings,
            Achievement_Plugins,
            Achievement_UltimateSimplicity,
            Achievement_KeepPatient,
            Achievement_MovePanel,
            Achievement_UndockPanel,
            Achievement_RemovePanel,
            Achievement_NewPanel,
            Achievement_QuickJump,
            Achievement_9bang15Pence,
            Achievement_42
        };
        Q_ENUM(Achievement)
        Q_INVOKABLE static void triggerAchievementCompleted(Achievement achievement);

    Q_SIGNALS:
        void achievementCompleted(const QString &id);

    private:
        friend class CorePlugin;
        explicit CoreAchievementsModel(QObject *parent = nullptr);
    };

}


#endif //COREACHIEVEMENTSMODEL_H
