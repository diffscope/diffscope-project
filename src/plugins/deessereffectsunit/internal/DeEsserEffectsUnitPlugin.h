// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSEREFFECTSUNITPLUGIN_H
#define DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSEREFFECTSUNITPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace DeEsserEffectsUnit::Internal {

    class DeEsserEffectsUnitPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        DeEsserEffectsUnitPlugin();
        ~DeEsserEffectsUnitPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSEREFFECTSUNITPLUGIN_H
