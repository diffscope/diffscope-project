// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EqualizerEffectsUnitPlugin.h"

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <audio/EffectsUnitCollection.h>

#include <equalizereffectsunit/internal/EqualizerEffectsUnit.h>

namespace EqualizerEffectsUnit::Internal {

    EqualizerEffectsUnitPlugin::EqualizerEffectsUnitPlugin() = default;

    EqualizerEffectsUnitPlugin::~EqualizerEffectsUnitPlugin() = default;

    bool EqualizerEffectsUnitPlugin::initialize(const QStringList &,
                                                QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        auto collection = Audio::EffectsUnitCollection::instance();
        Q_ASSERT(collection);
        auto effectsUnitClass = new EqualizerEffectsUnitClass(this);
        if (!collection || !collection->registerEffectsUnitClass(
                QStringLiteral("org.diffscope.equalizer"), effectsUnitClass)) {
            if (errorMessage) {
                *errorMessage = tr("The equalizer effect ID is already registered.");
            }
            return false;
        }
        return true;
    }

    void EqualizerEffectsUnitPlugin::extensionsInitialized() {
    }

    bool EqualizerEffectsUnitPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
