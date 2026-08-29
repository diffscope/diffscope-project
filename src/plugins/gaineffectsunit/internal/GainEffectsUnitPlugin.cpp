// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GainEffectsUnitPlugin.h"

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <audio/EffectsUnitCollection.h>

#include <gaineffectsunit/internal/GainEffectsUnit.h>

namespace GainEffectsUnit::Internal {

    GainEffectsUnitPlugin::GainEffectsUnitPlugin() = default;

    GainEffectsUnitPlugin::~GainEffectsUnitPlugin() = default;

    bool GainEffectsUnitPlugin::initialize(const QStringList &, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        auto collection = Audio::EffectsUnitCollection::instance();
        Q_ASSERT(collection);
        auto effectsUnitClass = new GainEffectsUnitClass(this);
        if (!collection || !collection->registerEffectsUnitClass(
                QStringLiteral("org.diffscope.gain"), effectsUnitClass)) {
            if (errorMessage) {
                *errorMessage = tr("The gain effect ID is already registered.");
            }
            return false;
        }
        return true;
    }

    void GainEffectsUnitPlugin::extensionsInitialized() {
    }

    bool GainEffectsUnitPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
