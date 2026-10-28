// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioPlugin.h"

#include <QApplication>
#include <QDir>
#include <QPointer>
#include <QQmlComponent>
#include <QSplashScreen>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>
#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/MessageBox.h>

#include <TalcsDevice/AudioDriverManager.h>
#include <TalcsDevice/OutputContext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/NotificationMessage.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/EffectsUnitCollection.h>
#include <audio/MidiInputContext.h>
#include <audio/internal/AudioAndMidiPage.h>
#include <audio/internal/AudioClipAddOn.h>
#include <audio/internal/AudioExporterPresets.h>
#include <audio/internal/AudioOutputPage.h>
#include <audio/internal/AudioPreference.h>
#include <audio/internal/AudioSystem.h>
#include <audio/internal/EffectsAddOn.h>
#include <audio/internal/EffectsExportListener.h>
#include <audio/internal/EffectsPresets.h>
#include <audio/internal/ExportAudioAddOn.h>
#include <audio/internal/ExportPage.h>
#include <audio/internal/MidiPage.h>
#include <audio/internal/MidiRecordingAddOn.h>
#include <audio/internal/OutputSystem.h>
#include <audio/internal/PlaybackAddOn.h>
#include <audio/internal/PlaybackPage.h>
#include <audio/internal/ProjectAudioAddOn.h>
#include <audio/internal/WaveformSingerAddOn.h>
#include <audio/private/GlobalAudioContext_p.h>

#include <org.diffscope.audio_actions.qak.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAudioPlugin, "diffscope.audio.audioplugin")

    AudioPlugin::AudioPlugin() {
    }
    AudioPlugin::~AudioPlugin() {
        delete m_midiInputContext;
    }

    bool AudioPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::audioActionExtension());
        Core::RuntimeInterface::splash()->showMessage(tr("Initializing audio plugin..."));
        qCInfo(lcAudioPlugin) << "Initializing";
        initializeAudioPreference();
        initializeAudioExporterPresets();
        initializeEffects();
        initializeAudioSystem();
        initializeSettings();
        initializePropertyEditors();
        initializeHelpContents();
        initializeWindows();
        qCInfo(lcAudioPlugin) << "Initialized";
        return true;
    }
    void AudioPlugin::extensionsInitialized() {
        initializeAudioOutput();
        m_midiInputContext->initialize();
        auto message = new Core::NotificationMessage(this);
        message->setIcon(SVS::SVSCraft::Information);
        message->setTitle(tr("Audio Device Changed"));
        message->setText(tr("A change in audio devices has been detected. You can check or adjust the current device in the audio settings."));
        message->setButtons({tr("Open Audio Output Settings")});
        message->setPrimaryButton(0);
        connect(message, &Core::NotificationMessage::buttonClicked, message, [message = QPointer<Core::NotificationMessage>(message)](int index) {
            if (index == 0) {
                Core::CoreInterface::execSettingsDialog(QStringLiteral("audio.AudioOutput"), QApplication::focusWindow());
                if (message) {
                    message->close();
                }
            }
        }, Qt::QueuedConnection);
        connect(AudioSystem::outputSystem()->outputContext(), &talcs::OutputContext::deviceHotPlugged, this, [message] {
            message->close();
            Core::CoreInterface::sendNotification(message, Core::CoreInterface::AutoHide);
        });
    }
    bool AudioPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
    QObject *AudioPlugin::remoteCommand(const QStringList &options, const QString &workingDirectory, const QStringList &args) {
        return IPlugin::remoteCommand(options, workingDirectory, args);
    }

    void AudioPlugin::initializeAudioPreference() {
        auto audioPreference = new AudioPreference(this);;
        audioPreference->load();
    }

    void AudioPlugin::initializeAudioExporterPresets() {
        auto audioExporterPresets = new AudioExporterPresets(this);
        audioExporterPresets->load();
    }

    void AudioPlugin::initializeEffects() {
        new EffectsUnitCollection(this);
        auto presets = new EffectsPresets(this);
        presets->load();
        EffectsExportListener::instance();
    }

    void AudioPlugin::initializeAudioSystem() {
        new AudioSystem(this);
#ifdef Q_OS_LINUX
        const auto directory = QDir(pluginSpec()->location()).absoluteFilePath(QStringLiteral(AUDIO_TALCS_LIBRARY_RELATIVE_DIR));
        AudioSystem::outputSystem()->outputContext()->driverManager()->loadBuiltInDriverLibraries(directory);
#endif
        GlobalAudioContextPrivate::create(this);
        m_midiInputContext = new MidiInputContext(this);
        Core::RuntimeInterface::instance()->addObject(m_midiInputContext);
    }

    void AudioPlugin::initializeAudioOutput() {
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
        audioAndMidiPage->addPage(new PlaybackPage);
        audioAndMidiPage->addPage(new MidiPage);
        audioAndMidiPage->addPage(new ExportPage);
        sc->addPage(audioAndMidiPage);
    }

    void AudioPlugin::initializePropertyEditors() {
        auto component = new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "AudioPropertyEditor", this);
        if (component->isError()) {
            qFatal() << component->errorString();
        }
        Core::RuntimeInterface::instance()->addObject("org.diffscope.core.propertyeditor.clip", component);
    }

    void AudioPlugin::initializeHelpContents() {
        auto component = new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "AudioOutputWelcomeWizardPage", this);
        if (component->isError()) {
            qFatal() << component->errorString();
        }
        Core::CoreInterface::actionContext()->addAction("org.diffscope.audio.welcomeWizard.audioOutput", component);
    }

    void AudioPlugin::initializeWindows() {
        Core::ProjectWindowInterfaceRegistry::instance()->attach<ProjectAudioAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<EffectsAddOn>();
        if (AudioSystem::isSingerRegistrationSuccessful()) {
            Core::ProjectWindowInterfaceRegistry::instance()->attach<WaveformSingerAddOn>();
        }
        Core::ProjectWindowInterfaceRegistry::instance()->attach<AudioClipAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<ExportAudioAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<PlaybackAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<MidiRecordingAddOn>();
    }
}
