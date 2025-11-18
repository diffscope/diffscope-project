#include "ImportExportManagerPlugin.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <importexportmanager/ConverterCollection.h>
#include <importexportmanager/internal/ImportAddOn.h>
#include <importexportmanager/internal/ProjectAddOn.h>

static auto getImportExportManagerActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(importexportmanager);
}

namespace ImportExportManager::Internal {

    ImportExportManagerPlugin::ImportExportManagerPlugin() {
    }

    ImportExportManagerPlugin::~ImportExportManagerPlugin() = default;

    bool ImportExportManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::getImportExportManagerActionExtension());
        new ConverterCollection(this);
        Core::HomeWindowInterfaceRegistry::instance()->attach<ImportAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<ImportAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<ProjectAddOn>();
        return true;
    }
    void ImportExportManagerPlugin::extensionsInitialized() {
    }
    bool ImportExportManagerPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
}
