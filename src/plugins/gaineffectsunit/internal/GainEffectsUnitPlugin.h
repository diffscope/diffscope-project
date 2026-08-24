// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_GAIN_EFFECTS_UNIT_GAINEFFECTSUNITPLUGIN_H
#define DIFFSCOPE_GAIN_EFFECTS_UNIT_GAINEFFECTSUNITPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace GainEffectsUnit::Internal {

    class GainEffectsUnitPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        GainEffectsUnitPlugin();
        ~GainEffectsUnitPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_GAIN_EFFECTS_UNIT_GAINEFFECTSUNITPLUGIN_H
