#ifndef DIFFSCOPE_ACHIEVEMENT_ACHIEVEMENTADDON_H
#define DIFFSCOPE_ACHIEVEMENT_ACHIEVEMENTADDON_H

#include <CoreApi/iwindow.h>
#include <qqmlintegration.h>

#include <QQuickWindow>

namespace Achievement {

    class AchievementAddOn : public Core::IWindowAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QQuickWindow *window READ window CONSTANT)
   public:
        explicit AchievementAddOn(QObject *parent = nullptr);
        ~AchievementAddOn() override;

        static QQuickWindow *window();
        static void setWindow(QQuickWindow *w);

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_ACHIEVEMENT_ACHIEVEMENTADDON_H
