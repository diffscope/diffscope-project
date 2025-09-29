#include "maintenanceplugin.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>
#include <CoreApi/settingcatalog.h>

#include <extensionsystem/pluginspec.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/homewindowinterface.h>
#include <coreplugin/projectwindowinterface.h>

#include <maintenance/internal/maintenanceaddon.h>
#include <maintenance/internal/applicationupdatechecker.h>
#include <maintenance/internal/updatepage.h>

static auto getMaintenanceActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(maintenance_actions);
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

        if (auto generalPage = Core::CoreInterface::settingCatalog()->page("core.General")) {
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
