#include "MIDIFormatConverterPlugin.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <importexportmanager/ConverterCollection.h>

#include <midiformatconverter/internal/MIDIFileImporter.h>
#include <midiformatconverter/internal/MIDIFileExporter.h>

namespace MIDIFormatConverter::Internal {

    MIDIFormatConverterPlugin::MIDIFormatConverterPlugin() {
    }

    MIDIFormatConverterPlugin::~MIDIFormatConverterPlugin() = default;

    bool MIDIFormatConverterPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        ImportExportManager::ConverterCollection::instance()->addObject(new MIDIFileImporter);
        ImportExportManager::ConverterCollection::instance()->addObject(new MIDIFileExporter);
        return true;
    }

    void MIDIFormatConverterPlugin::extensionsInitialized() {
    }

    bool MIDIFormatConverterPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
