#include "LibreSVIPFormatConverterPlugin.h"

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <importexportmanager/ConverterCollection.h>

#include <QtProtobuf/qprotobufregistration.h>

#include <coreplugin/CoreInterface.h>

#include <libresvipformatconverter/internal/LibreSVIPFileImporter.h>
#include <libresvipformatconverter/internal/LibreSVIPFileExporter.h>
#include <libresvipformatconverter/internal/LibreSVIPManager.h>
#include <libresvipformatconverter/internal/LibreSVIPSettingsPage.h>

#include "libresvip.qpb.h"

namespace LibreSVIPFormatConverter::Internal {

    static void registerLibreSVIPProtobufTypes() {
        qRegisterProtobufTypes();
        qRegisterProtobufType<LibreSVIP::PluginInfo>();
        qRegisterProtobufType<LibreSVIP::PluginInfosRequest>();
        qRegisterProtobufType<LibreSVIP::PluginInfosResponse>();
        qRegisterProtobufType<LibreSVIP::ConversionGroup>();
        qRegisterProtobufType<LibreSVIP::SingleConversionResult>();
        qRegisterProtobufType<LibreSVIP::ConversionRequest>();
        qRegisterProtobufType<LibreSVIP::ConversionResponse>();
    }

    LibreSVIPFormatConverterPlugin::LibreSVIPFormatConverterPlugin() {
    }

    LibreSVIPFormatConverterPlugin::~LibreSVIPFormatConverterPlugin() = default;

    bool LibreSVIPFormatConverterPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        registerLibreSVIPProtobufTypes();
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        auto *manager = new LibreSVIPManager(this);
        Core::CoreInterface::settingCatalog()->addPage(new LibreSVIPSettingsPage(manager));
        ImportExportManager::ConverterCollection::addFileConverter(new LibreSVIPFileImporter(manager));
        ImportExportManager::ConverterCollection::addFileConverter(new LibreSVIPFileExporter(manager));
        return true;
    }

    void LibreSVIPFormatConverterPlugin::extensionsInitialized() {
    }

    bool LibreSVIPFormatConverterPlugin::delayedInitialize() {
        LibreSVIPManager::instance()->checkForUpdates();
        return IPlugin::delayedInitialize();
    }

}
