#include "welcomewizardplugin.h"

#include <QSettings>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/ihomewindow.h>
#include <coreplugin/iprojectwindow.h>

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
        Core::ICore::actionRegistry()->addExtension(::getWelcomeWizardActionExtension());
        Core::IHomeWindowRegistry::instance()->attach<WelcomeWizardAddOn>();
        Core::IProjectWindowRegistry::instance()->attach<WelcomeWizardAddOn>();
        return true;
    }
    void WelcomeWizardPlugin::extensionsInitialized() {
    }
    bool WelcomeWizardPlugin::delayedInitialize() {
        auto settings = Core::PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        if (!settings->value("welcomeWizardShown", false).toBool()) {
            WelcomeWizardAddOn::execWelcomeWizard();
            settings->setValue("welcomeWizardShown", true);
        }
        settings->endGroup();
        return IPlugin::delayedInitialize();
    }
}