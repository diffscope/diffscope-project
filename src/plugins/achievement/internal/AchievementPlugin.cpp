#include "AchievementPlugin.h"

#include <QQmlComponent>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <achievement/internal/achievementaddon.h>

static auto getAchievementActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(achievement);
}

namespace Achievement {

    AchievementPlugin::AchievementPlugin() {
    }

    AchievementPlugin::~AchievementPlugin() = default;

    bool AchievementPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::getAchievementActionExtension());
        Core::HomeWindowInterfaceRegistry::instance()->attach<AchievementAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<AchievementAddOn>();
        auto component = new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.Achievement", "AchievementWelcomeWizardPage", this);
        if (component->isError()) {
            qFatal() << component->errorString();
        }
        Core::RuntimeInterface::instance()->addObject("org.diffscope.welcomewizard.pages", component);
        return true;
    }
    void AchievementPlugin::extensionsInitialized() {
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.UIShell", "AchievementDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties(
            {{"models", QVariant::fromValue(Core::RuntimeInterface::instance()->getObjects("org.diffscope.achievements"))},
             {"settings", QVariant::fromValue(Core::RuntimeInterface::settings())},
             {"settingCategory", "org.diffscope.achievements"}}
        );
        o->setParent(this);
        AchievementAddOn::setWindow(qobject_cast<QQuickWindow *>(o));
    }
    bool AchievementPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
}
