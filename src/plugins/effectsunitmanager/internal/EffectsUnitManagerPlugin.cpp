// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EffectsUnitManagerPlugin.h"

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <effectsunitmanager/EffectsUnitCollection.h>
#include <effectsunitmanager/internal/EffectsPanelAddOn.h>
#include <effectsunitmanager/internal/EffectsPresets.h>

static auto effectsUnitManagerActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(effectsunitmanager);
}

namespace EffectsUnitManager::Internal {

    EffectsUnitManagerPlugin::EffectsUnitManagerPlugin() = default;

    EffectsUnitManagerPlugin::~EffectsUnitManagerPlugin() = default;

    bool EffectsUnitManagerPlugin::initialize(const QStringList &, QString *) {
        new EffectsUnitCollection(this);
        auto presets = new EffectsPresets(this);
        presets->load();
        Core::CoreInterface::actionRegistry()->addExtension(::effectsUnitManagerActionExtension());
        Core::ProjectWindowInterfaceRegistry::instance()->attach<EffectsPanelAddOn>();
        return true;
    }

    void EffectsUnitManagerPlugin::extensionsInitialized() {
    }

    bool EffectsUnitManagerPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
