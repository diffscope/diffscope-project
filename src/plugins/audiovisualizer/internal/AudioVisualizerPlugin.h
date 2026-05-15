#ifndef DIFFSCOPE_AUDIO_VISUALIZER_AUDIOVISUALIZERPLUGIN_H
#define DIFFSCOPE_AUDIO_VISUALIZER_AUDIOVISUALIZERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace AudioVisualizer::Internal {

    class AudioVisualizerPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        AudioVisualizerPlugin();
        ~AudioVisualizerPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        static void initializeWindows();
    };

}

#endif // DIFFSCOPE_AUDIO_VISUALIZER_AUDIOVISUALIZERPLUGIN_H
