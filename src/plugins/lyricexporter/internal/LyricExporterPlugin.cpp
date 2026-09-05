// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "LyricExporterPlugin.h"

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <importexportmanager/ConverterCollection.h>

#include <lyricexporter/internal/LyricFileExporter.h>

namespace LyricExporter::Internal {

    LyricExporterPlugin::LyricExporterPlugin() = default;

    LyricExporterPlugin::~LyricExporterPlugin() = default;

    bool LyricExporterPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(
            pluginSpec()->location() + QStringLiteral("/translations")
        );
        ImportExportManager::ConverterCollection::addFileConverter(new LyricFileExporter);
        return true;
    }

    void LyricExporterPlugin::extensionsInitialized() {
    }

    bool LyricExporterPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
