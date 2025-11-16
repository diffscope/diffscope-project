#include "WelcomeWizardPlugin.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <welcomewizard/internal/welcomewizardaddon.h>

static auto getWelcomeWizardActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(welcomewizard);
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
        bool shown = settings->value("welcomeWizardShown", false).toBool();
        settings->endGroup();
        if (!shown) {
            WelcomeWizardAddOn::execWelcomeWizard();
            settings->beginGroup(staticMetaObject.className());
            settings->setValue("welcomeWizardShown", true);
            settings->endGroup();
        }
        return IPlugin::delayedInitialize();
    }
}
