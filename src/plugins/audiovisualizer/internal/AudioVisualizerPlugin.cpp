#include "AudioVisualizerPlugin.h"

#include "addon/AudioMipmapAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <extensionsystem/pluginspec.h>

static auto getAudioVisualizerActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(audiovisualizer);
}

namespace AudioVisualizer::Internal {

    AudioVisualizerPlugin::AudioVisualizerPlugin() {
    }

    AudioVisualizerPlugin::~AudioVisualizerPlugin() = default;

    bool AudioVisualizerPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::getAudioVisualizerActionExtension());
        auto audioThumbnailComponent = new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.AudioVisualizer", "AudioThumbnail", this);
        if (audioThumbnailComponent->isError()) {
            qFatal() << audioThumbnailComponent->errorString();
        }
        Core::RuntimeInterface::instance()->addObject("org.diffscope.visualeditor.audiothumbnailcomponent", audioThumbnailComponent);
        Core::ProjectWindowInterfaceRegistry::instance()->attach<AudioMipmapAddOn>();

        return true;
    }

    void AudioVisualizerPlugin::extensionsInitialized() {
    }

    bool AudioVisualizerPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
