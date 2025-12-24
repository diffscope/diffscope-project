#include "MaintenancePlugin.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <maintenance/internal/ApplicationUpdateChecker.h>
#include <maintenance/internal/MaintenanceAddOn.h>
#include <maintenance/internal/UpdatePage.h>

static auto getMaintenanceActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(maintenance);
}

namespace Maintenance {

    MaintenancePlugin::MaintenancePlugin() {
        MaintenanceAddOn::setPlugin(this);
    }

    MaintenancePlugin::~MaintenancePlugin() = default;

    bool MaintenancePlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::getMaintenanceActionExtension());
        Core::HomeWindowInterfaceRegistry::instance()->attach<MaintenanceAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<MaintenanceAddOn>();

        new ApplicationUpdateChecker(this);

        if (auto generalPage = Core::CoreInterface::settingCatalog()->page("org.diffscope.core.General")) {
            generalPage->addPage(new UpdatePage);
        } else {
            Core::CoreInterface::settingCatalog()->addPage(new UpdatePage);
        }

        return true;
    }
    void MaintenancePlugin::extensionsInitialized() {
    }
    bool MaintenancePlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
}
