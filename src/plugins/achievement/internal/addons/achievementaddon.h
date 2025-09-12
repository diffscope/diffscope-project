#ifndef DIFFSCOPE_ACHIEVEMENT_ACHIEVEMENTADDON_H
#define DIFFSCOPE_ACHIEVEMENT_ACHIEVEMENTADDON_H

#include <CoreApi/iwindow.h>

namespace Achievement {

    class AchievementAddOn : public Core::IWindowAddOn {
        Q_OBJECT
   public:
        explicit AchievementAddOn(QObject *parent = nullptr);
        ~AchievementAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_ACHIEVEMENT_ACHIEVEMENTADDON_H
