// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSOREFFECTSUNITPLUGIN_H
#define DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSOREFFECTSUNITPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace CompressorEffectsUnit::Internal {

    class CompressorEffectsUnitPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        CompressorEffectsUnitPlugin();
        ~CompressorEffectsUnitPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSOREFFECTSUNITPLUGIN_H
