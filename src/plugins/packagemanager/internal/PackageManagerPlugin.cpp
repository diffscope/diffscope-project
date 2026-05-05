#include "PackageManagerPlugin.h"

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <extensionsystem/pluginspec.h>

#include <packagemanager/internal/PackageManagerSettings.h>
#include <packagemanager/internal/PackageManagerAddOn.h>
#include <packagemanager/internal/PackageManagerPage.h>

static auto getPackageManagerActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(packagemanager);
}

namespace PackageManager {

    PackageManagerPlugin::PackageManagerPlugin() = default;

    PackageManagerPlugin::~PackageManagerPlugin() = default;

    bool PackageManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Q_UNUSED(arguments)
        Q_UNUSED(errorMessage)

        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        new PackageManagerSettings(pluginSpec()->location(), this);

        Core::CoreInterface::actionRegistry()->addExtension(::getPackageManagerActionExtension());
        Core::HomeWindowInterfaceRegistry::instance()->attach<PackageManagerAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<PackageManagerAddOn>();

        Core::CoreInterface::settingCatalog()->addPage(new PackageManagerPage);

        return true;
    }

    void PackageManagerPlugin::extensionsInitialized() {
    }

    bool PackageManagerPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
