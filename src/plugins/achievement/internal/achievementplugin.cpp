#include "achievementplugin.h"

#include <QQmlComponent>

#include <QAKCore/actionregistry.h>

#include <CoreApi/plugindatabase.h>

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
        QQmlComponent component(Core::PluginDatabase::qmlEngine(), "DiffScope.UIShell", "AchievementDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"models", QVariant::fromValue(Core::PluginDatabase::instance()->getObjects("org.diffscope.achievements"))},
            {"settings", QVariant::fromValue(Core::PluginDatabase::settings())},
            {"settingCategory", "org.diffscope.achievements"}
        });
        o->setParent(this);
        AchievementAddOn::setWindow(qobject_cast<QQuickWindow *>(o));
    }
    bool AchievementPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
}