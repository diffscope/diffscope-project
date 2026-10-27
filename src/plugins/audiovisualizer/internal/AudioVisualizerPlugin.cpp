// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioVisualizerPlugin.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audiovisualizer/internal/AudioMipmapAddOn.h>
#include <audiovisualizer/internal/LevelMeterAddOn.h>
#include <audiovisualizer/internal/MasterTrackAddOn.h>
#include <audiovisualizer/internal/MidiRecordingQuantizationAddOn.h>

#include <org.diffscope.audiovisualizer_actions.qak.h>

namespace AudioVisualizer::Internal {

    AudioVisualizerPlugin::AudioVisualizerPlugin() {
    }

    AudioVisualizerPlugin::~AudioVisualizerPlugin() = default;

    bool AudioVisualizerPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::audiovisualizerActionExtension());
        auto audioThumbnailComponent = new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.AudioVisualizer", "AudioThumbnail", this);
        if (audioThumbnailComponent->isError()) {
            qFatal() << audioThumbnailComponent->errorString();
        }
        Core::RuntimeInterface::instance()->addObject("org.diffscope.visualeditor.audiothumbnailcomponent", audioThumbnailComponent);
        Core::ProjectWindowInterfaceRegistry::instance()->attach<AudioMipmapAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<MasterTrackAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<LevelMeterAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<MidiRecordingQuantizationAddOn>();

        return true;
    }

    void AudioVisualizerPlugin::extensionsInitialized() {
    }

    bool AudioVisualizerPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
