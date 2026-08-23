// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ChannelMapperEffectsUnitPlugin.h"

#include <effectsunitmanager/EffectsUnitCollection.h>

#include <channelmappereffectsunit/internal/ChannelMapperEffectsUnit.h>

namespace ChannelMapperEffectsUnit::Internal {

    ChannelMapperEffectsUnitPlugin::ChannelMapperEffectsUnitPlugin() = default;

    ChannelMapperEffectsUnitPlugin::~ChannelMapperEffectsUnitPlugin() = default;

    bool ChannelMapperEffectsUnitPlugin::initialize(const QStringList &, QString *errorMessage) {
        auto collection = EffectsUnitManager::EffectsUnitCollection::instance();
        Q_ASSERT(collection);
        auto effectsUnitClass = new ChannelMapperEffectsUnitClass(this);
        if (!collection || !collection->registerEffectsUnitClass(
                QStringLiteral("org.diffscope.channelmapper"), effectsUnitClass)) {
            if (errorMessage) {
                *errorMessage = tr("The channel mapper effect ID is already registered.");
            }
            return false;
        }
        return true;
    }

    void ChannelMapperEffectsUnitPlugin::extensionsInitialized() {
    }

    bool ChannelMapperEffectsUnitPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
