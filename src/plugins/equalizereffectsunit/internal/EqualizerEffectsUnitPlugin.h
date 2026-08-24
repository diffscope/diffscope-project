// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZEREFFECTSUNITPLUGIN_H
#define DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZEREFFECTSUNITPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace EqualizerEffectsUnit::Internal {

    class EqualizerEffectsUnitPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        EqualizerEffectsUnitPlugin();
        ~EqualizerEffectsUnitPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZEREFFECTSUNITPLUGIN_H
