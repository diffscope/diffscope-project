// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_CHANNEL_MAPPER_EFFECTS_UNIT_CHANNELMAPPEREFFECTSUNITPLUGIN_H
#define DIFFSCOPE_CHANNEL_MAPPER_EFFECTS_UNIT_CHANNELMAPPEREFFECTSUNITPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace ChannelMapperEffectsUnit::Internal {

    class ChannelMapperEffectsUnitPlugin final : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        ChannelMapperEffectsUnitPlugin();
        ~ChannelMapperEffectsUnitPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_CHANNEL_MAPPER_EFFECTS_UNIT_CHANNELMAPPEREFFECTSUNITPLUGIN_H
