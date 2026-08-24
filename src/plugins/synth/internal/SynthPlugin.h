// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_SYNTHPLUGIN_H
#define DIFFSCOPE_SYNTH_INTERNAL_SYNTHPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Synth::Internal {

    class SynthService;

    class SynthPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        SynthPlugin();
        ~SynthPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
        ShutdownFlag aboutToShutdown() override;

    private:
        SynthService *m_service{};
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_SYNTHPLUGIN_H
