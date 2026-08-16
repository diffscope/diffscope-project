// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_VISUALIZER_SYNTHVISUALIZERPLUGIN_H
#define DIFFSCOPE_SYNTH_VISUALIZER_SYNTHVISUALIZERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace SynthVisualizer::Internal {

    class SynthVisualizerPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        SynthVisualizerPlugin();
        ~SynthVisualizerPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_SYNTH_VISUALIZER_SYNTHVISUALIZERPLUGIN_H
