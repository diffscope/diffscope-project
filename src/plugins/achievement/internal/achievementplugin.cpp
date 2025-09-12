#include "achievementplugin.h"

#include <QAKCore/actionregistry.h>

#include <coreplugin/icore.h>
#include <coreplugin/ihomewindow.h>
#include <coreplugin/iprojectwindow.h>

#include <achievement/internal/achievementaddon.h>

static auto getAchievementActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(achievement_actions);
}

namespace Achievement {

    AchievementPlugin::AchievementPlugin() {
    }
    AchievementPlugin::~AchievementPlugin() = default;
    bool AchievementPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::ICore::actionRegistry()->addExtension(::getAchievementActionExtension());
        Core::IHomeWindowRegistry::instance()->attach<AchievementAddOn>();
        Core::IProjectWindowRegistry::instance()->attach<AchievementAddOn>();
        return true;
    }
    void AchievementPlugin::extensionsInitialized() {
    }
    bool AchievementPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
}