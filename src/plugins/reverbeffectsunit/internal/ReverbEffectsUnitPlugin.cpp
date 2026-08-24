// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ReverbEffectsUnitPlugin.h"

#include <audio/EffectsUnitCollection.h>

#include <reverbeffectsunit/internal/ReverbEffectsUnit.h>

namespace ReverbEffectsUnit::Internal {

    ReverbEffectsUnitPlugin::ReverbEffectsUnitPlugin() = default;

    ReverbEffectsUnitPlugin::~ReverbEffectsUnitPlugin() = default;

    bool ReverbEffectsUnitPlugin::initialize(const QStringList &, QString *errorMessage) {
        auto collection = Audio::EffectsUnitCollection::instance();
        Q_ASSERT(collection);
        auto effectsUnitClass = new ReverbEffectsUnitClass(this);
        if (!collection || !collection->registerEffectsUnitClass(
                QStringLiteral("org.diffscope.reverb"), effectsUnitClass)) {
            if (errorMessage) {
                *errorMessage = tr("The reverb effect ID is already registered.");
            }
            return false;
        }
        return true;
    }

    void ReverbEffectsUnitPlugin::extensionsInitialized() {
    }

    bool ReverbEffectsUnitPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
