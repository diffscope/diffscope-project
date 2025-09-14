#include "welcomewizardplugin.h"

#include <QQmlComponent>

#include <QAKCore/actionregistry.h>

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
    }
    WelcomeWizardPlugin::~WelcomeWizardPlugin() = default;
    bool WelcomeWizardPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::ICore::actionRegistry()->addExtension(::getWelcomeWizardActionExtension());
        Core::IHomeWindowRegistry::instance()->attach<WelcomeWizardAddOn>();
        Core::IProjectWindowRegistry::instance()->attach<WelcomeWizardAddOn>();
        return true;
    }
    void WelcomeWizardPlugin::extensionsInitialized() {
        // Note: WelcomeWizard dialog implementation is not yet complete
        // This is where the welcome wizard window would be created and configured
        // when the wizard implementation is ready
    }
    bool WelcomeWizardPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
}