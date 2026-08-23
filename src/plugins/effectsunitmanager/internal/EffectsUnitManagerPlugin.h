// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITMANAGERPLUGIN_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITMANAGERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace EffectsUnitManager::Internal {

    class EffectsUnitManagerPlugin final : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        EffectsUnitManagerPlugin();
        ~EffectsUnitManagerPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITMANAGERPLUGIN_H
