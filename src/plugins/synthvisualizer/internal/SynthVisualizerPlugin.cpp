// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthVisualizerPlugin.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <synthvisualizer/internal/SynthesisPieceTrackAddOn.h>

static auto synthVisualizerActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(synthvisualizer);
}

namespace SynthVisualizer::Internal {

    SynthVisualizerPlugin::SynthVisualizerPlugin() = default;
    SynthVisualizerPlugin::~SynthVisualizerPlugin() = default;

    bool SynthVisualizerPlugin::initialize(const QStringList &, QString *) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(
            pluginSpec()->location() + QStringLiteral("/translations")
        );
        Core::CoreInterface::actionRegistry()->addExtension(::synthVisualizerActionExtension());
        auto phonemePanelBackgroundComponent = new QQmlComponent(
            Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.SynthVisualizer"),
            QStringLiteral("SynthesisWaveformBackground"), this
        );
        if (phonemePanelBackgroundComponent->isError()) {
            qFatal() << phonemePanelBackgroundComponent->errorString();
        }
        Core::RuntimeInterface::instance()->addObject(
            QStringLiteral("org.diffscope.visualeditor.phonemepanelbackgroundcomponent"),
            phonemePanelBackgroundComponent
        );
        Core::ProjectWindowInterfaceRegistry::instance()->attach<SynthesisPieceTrackAddOn>();
        return true;
    }

    void SynthVisualizerPlugin::extensionsInitialized() {
    }

    bool SynthVisualizerPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
