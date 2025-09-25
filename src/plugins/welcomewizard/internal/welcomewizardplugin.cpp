#include "welcomewizardplugin.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/homewindowinterface.h>
#include <coreplugin/projectwindowinterface.h>

#include <welcomewizard/internal/welcomewizardaddon.h>

static auto getWelcomeWizardActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(welcome_wizard_actions);
}

namespace WelcomeWizard {

    WelcomeWizardPlugin::WelcomeWizardPlugin() {
        WelcomeWizardAddOn::setPlugin(this);
    }

    WelcomeWizardPlugin::~WelcomeWizardPlugin() = default;

    bool WelcomeWizardPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::getWelcomeWizardActionExtension());
        Core::HomeWindowInterfaceRegistry::instance()->attach<WelcomeWizardAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<WelcomeWizardAddOn>();
        return true;
    }
    void WelcomeWizardPlugin::extensionsInitialized() {
    }
    bool WelcomeWizardPlugin::delayedInitialize() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        if (!settings->value("welcomeWizardShown", false).toBool()) {
            WelcomeWizardAddOn::execWelcomeWizard();
            settings->setValue("welcomeWizardShown", true);
        }
        settings->endGroup();
        return IPlugin::delayedInitialize();
    }
}
