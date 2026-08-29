// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DeEsserEffectsUnitPlugin.h"

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <audio/EffectsUnitCollection.h>

#include <deessereffectsunit/internal/DeEsserEffectsUnit.h>

namespace DeEsserEffectsUnit::Internal {

    DeEsserEffectsUnitPlugin::DeEsserEffectsUnitPlugin() = default;

    DeEsserEffectsUnitPlugin::~DeEsserEffectsUnitPlugin() = default;

    bool DeEsserEffectsUnitPlugin::initialize(const QStringList &,
                                              QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        auto collection = Audio::EffectsUnitCollection::instance();
        Q_ASSERT(collection);
        auto effectsUnitClass = new DeEsserEffectsUnitClass(this);
        if (!collection || !collection->registerEffectsUnitClass(
                QStringLiteral("org.diffscope.deesser"), effectsUnitClass)) {
            if (errorMessage) {
                *errorMessage = tr("The de-esser effect ID is already registered.");
            }
            return false;
        }
        return true;
    }

    void DeEsserEffectsUnitPlugin::extensionsInitialized() {
    }

    bool DeEsserEffectsUnitPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
