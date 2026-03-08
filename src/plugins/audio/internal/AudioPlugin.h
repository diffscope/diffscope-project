#ifndef DIFFSCOPE_AUDIO_AUDIOPLUGIN_H
#define DIFFSCOPE_AUDIO_AUDIOPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Audio::Internal {

    class AudioPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        AudioPlugin();
        ~AudioPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QObject *remoteCommand(const QStringList &options, const QString &workingDirectory, const QStringList &args) override;

    private:
        void initializeAudioSystem();
        static void initializeSettings();
        void initializeHelpContents();
    };

}

#endif //DIFFSCOPE_AUDIO_AUDIOPLUGIN_H
