// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CompressorEffectsUnitPlugin.h"

#include <audio/EffectsUnitCollection.h>

#include <compressoreffectsunit/internal/CompressorEffectsUnit.h>

namespace CompressorEffectsUnit::Internal {

    CompressorEffectsUnitPlugin::CompressorEffectsUnitPlugin() = default;

    CompressorEffectsUnitPlugin::~CompressorEffectsUnitPlugin() = default;

    bool CompressorEffectsUnitPlugin::initialize(const QStringList &, QString *errorMessage) {
        auto collection = Audio::EffectsUnitCollection::instance();
        Q_ASSERT(collection);
        auto effectsUnitClass = new CompressorEffectsUnitClass(this);
        if (!collection || !collection->registerEffectsUnitClass(
                QStringLiteral("org.diffscope.compressor"), effectsUnitClass)) {
            if (errorMessage) {
                *errorMessage = tr("The compressor effect ID is already registered.");
            }
            return false;
        }
        return true;
    }

    void CompressorEffectsUnitPlugin::extensionsInitialized() {
    }

    bool CompressorEffectsUnitPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
