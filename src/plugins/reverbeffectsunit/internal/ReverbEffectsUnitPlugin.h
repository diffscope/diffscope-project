// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBEFFECTSUNITPLUGIN_H
#define DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBEFFECTSUNITPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace ReverbEffectsUnit::Internal {

    class ReverbEffectsUnitPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        ReverbEffectsUnitPlugin();
        ~ReverbEffectsUnitPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBEFFECTSUNITPLUGIN_H
