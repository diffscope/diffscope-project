// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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
#include <importexportmanager/internal/FileImportExportAddOn.h>
#include <importexportmanager/internal/DspxFileImporter.h>
#include <importexportmanager/internal/DspxFileExporter.h>

#include <org.diffscope.importexportmanager_actions.qak.h>

namespace ImportExportManager::Internal {

    ImportExportManagerPlugin::ImportExportManagerPlugin() {
    }

    ImportExportManagerPlugin::~ImportExportManagerPlugin() = default;

    bool ImportExportManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::importexportmanagerActionExtension());

        new ConverterCollection(this);

        Core::HomeWindowInterfaceRegistry::instance()->attach<FileImportExportAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<FileImportExportAddOn>();

        return true;
    }
    void ImportExportManagerPlugin::extensionsInitialized() {
        ConverterCollection::addFileConverter(new DspxFileImporter);
        ConverterCollection::addFileConverter(new DspxFileExporter);
    }
    bool ImportExportManagerPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
}
