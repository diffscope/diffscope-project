#ifndef DIFFSCOPE_ACHIEVEMENT_ACHIEVEMENTPLUGIN_H
#define DIFFSCOPE_ACHIEVEMENT_ACHIEVEMENTPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Achievement {

    class AchievementPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        AchievementPlugin();
        ~AchievementPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    };

}

#endif //DIFFSCOPE_ACHIEVEMENT_ACHIEVEMENTPLUGIN_H
