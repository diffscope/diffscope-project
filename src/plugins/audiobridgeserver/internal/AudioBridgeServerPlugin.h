// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOBRIDGESERVER_AUDIOBRIDGESERVERPLUGIN_H
#define DIFFSCOPE_AUDIOBRIDGESERVER_AUDIOBRIDGESERVERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace AudioBridge::Internal {

    class BridgeServer;

    class AudioBridgeServerPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        ShutdownFlag aboutToShutdown() override;

    private:
        BridgeServer *m_server{};
    };

}

#endif
