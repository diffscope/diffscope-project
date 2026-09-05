// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_LYRIC_EXPORTER_LYRICEXPORTERPLUGIN_H
#define DIFFSCOPE_LYRIC_EXPORTER_LYRICEXPORTERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace LyricExporter::Internal {

    class LyricExporterPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        LyricExporterPlugin();
        ~LyricExporterPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_LYRIC_EXPORTER_LYRICEXPORTERPLUGIN_H
