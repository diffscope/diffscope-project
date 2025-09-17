#include "audioplugin.h"

#include <QApplication>
#include <QSplashScreen>

#include <SVSCraftQuick/MessageBox.h>

#include <CoreApi/plugindatabase.h>
#include <CoreApi/settingcatalog.h>

#include <coreplugin/coreinterface.h>

#include <audio/internal/audiosystem.h>
#include <audio/internal/outputsystem.h>
#include <audio/internal/audioandmidipage.h>
#include <audio/internal/audiooutputpage.h>

namespace Audio::Internal {
    AudioPlugin::AudioPlugin() {
    }
    AudioPlugin::~AudioPlugin() = default;

    bool AudioPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::PluginDatabase::splash()->showMessage(tr("Initializing audio plugin..."));
        initializeSettings();
        initializeAudioSystem();
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

    void AudioPlugin::initializeAudioSystem() {
        new AudioSystem(this);
        auto outputSystemInitializationSuccess = AudioSystem::outputSystem()->initialize();
        if (!outputSystemInitializationSuccess) {
            SVS::MessageBox::warning(
                Core::PluginDatabase::qmlEngine(),
                nullptr,
                tr("Failed to initialize audio output system"),
                tr("%1 will not play sound because no available audio output device found.\n\nPlease go to Settings > Audio and MIDI > Audio Output to check the device status.").arg(QApplication::applicationName()));
        }
    }

    void AudioPlugin::initializeSettings() {
        auto sc = Core::CoreInterface::settingCatalog();
        auto audioAndMidiPage = new AudioAndMidiPage;
        audioAndMidiPage->addPage(new AudioOutputPage);
        sc->addPage(audioAndMidiPage);
    }
}
