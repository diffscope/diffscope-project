#include "AudioPlugin.h"

#include <QApplication>
#include <QQmlComponent>
#include <QSplashScreen>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/CoreInterface.h>

#include <audio/internal/AudioAndMidiPage.h>
#include <audio/internal/AudioOutputPage.h>
#include <audio/internal/AudioSystem.h>
#include <audio/internal/OutputSystem.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAudioPlugin, "diffscope.audio.audioplugin")

    AudioPlugin::AudioPlugin() {
    }
    AudioPlugin::~AudioPlugin() = default;

    bool AudioPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::RuntimeInterface::splash()->showMessage(tr("Initializing audio plugin..."));
        qCInfo(lcAudioPlugin) << "Initializing";
        initializeSettings();
        initializeAudioSystem();
        initializeHelpContents();
        qCInfo(lcAudioPlugin) << "Initialized";
        return true;
    }
    void AudioPlugin::extensionsInitialized() {
    }
    bool AudioPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
    QObject *AudioPlugin::remoteCommand(const QStringList &options, const QString &workingDirectory, const QStringList &args) {
        return IPlugin::remoteCommand(options, workingDirectory, args);
    }

    void AudioPlugin::initializeAudioSystem() {
        new AudioSystem(this);
        auto outputSystemInitializationSuccess = AudioSystem::outputSystem()->initialize();
        if (!outputSystemInitializationSuccess) {
            SVS::MessageBox::warning(
                Core::RuntimeInterface::qmlEngine(),
                nullptr,
                tr("Failed to initialize audio output system"),
                tr("%1 will not play sound because no available audio output device found.\n\nPlease go to Settings > Audio and MIDI > Audio Output to check the device status.").arg(QApplication::applicationDisplayName())
            );
        }
    }

    void AudioPlugin::initializeSettings() {
        auto sc = Core::CoreInterface::settingCatalog();
        auto audioAndMidiPage = new AudioAndMidiPage;
        audioAndMidiPage->addPage(new AudioOutputPage);
        sc->addPage(audioAndMidiPage);
    }

    void AudioPlugin::initializeHelpContents() {
        auto component = new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "AudioOutputWelcomeWizardPage", this);
        if (component->isError()) {
            qFatal() << component->errorString();
        }
        Core::RuntimeInterface::instance()->addObject("org.diffscope.welcomewizard.pages", component);
    }
}
