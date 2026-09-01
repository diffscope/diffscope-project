// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PitchShifterPlugin.h"

#include <QLoggingCategory>
#include <QSplashScreen>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <QAKCore/actionregistry.h>

#include <extensionsystem/pluginspec.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <pitchshifter/internal/PitchShiftAddOn.h>

static auto getPitchShifterActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(pitchshifter);
}

namespace PitchShifter::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcPitchShifterPlugin, "diffscope.pitchshifter.plugin")

    PitchShifterPlugin::PitchShifterPlugin() = default;

    PitchShifterPlugin::~PitchShifterPlugin() = default;

    bool PitchShifterPlugin::initialize(const QStringList &, QString *) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::getPitchShifterActionExtension());
        Core::RuntimeInterface::splash()->showMessage(tr("Initializing pitch shifter plugin..."));
        qCInfo(lcPitchShifterPlugin) << "Initializing";
        Core::ProjectWindowInterfaceRegistry::instance()->attach<PitchShiftAddOn>();
        qCInfo(lcPitchShifterPlugin) << "Initialized";
        return true;
    }

    void PitchShifterPlugin::extensionsInitialized() {
    }

    bool PitchShifterPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
