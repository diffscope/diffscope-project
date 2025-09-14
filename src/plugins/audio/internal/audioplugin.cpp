#include "audioplugin.h"


namespace Audio::Internal {
    AudioPlugin::AudioPlugin() {
    }
    AudioPlugin::~AudioPlugin() = default;
    bool AudioPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        return true;
    }
    void AudioPlugin::extensionsInitialized() {
    }
    bool AudioPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
    QObject *AudioPlugin::remoteCommand(const QStringList &options,
                                        const QString &workingDirectory,
                                        const QStringList &args) {
        return IPlugin::remoteCommand(options, workingDirectory, args);
    }
}