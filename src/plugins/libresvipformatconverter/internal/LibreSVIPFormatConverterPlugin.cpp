#include "LibreSVIPFormatConverterPlugin.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <importexportmanager/ConverterCollection.h>

#include <libresvipformatconverter/internal/LibreSVIPFileImporter.h>
#include <libresvipformatconverter/internal/LibreSVIPFileExporter.h>

namespace LibreSVIPFormatConverter::Internal {

    LibreSVIPFormatConverterPlugin::LibreSVIPFormatConverterPlugin() {
    }

    LibreSVIPFormatConverterPlugin::~LibreSVIPFormatConverterPlugin() = default;

    bool LibreSVIPFormatConverterPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        ImportExportManager::ConverterCollection::addFileConverter(new LibreSVIPFileImporter);
        ImportExportManager::ConverterCollection::addFileConverter(new LibreSVIPFileExporter);
        return true;
    }

    void LibreSVIPFormatConverterPlugin::extensionsInitialized() {
    }

    bool LibreSVIPFormatConverterPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
