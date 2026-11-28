// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioBridgeServerPlugin.h"

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>
#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audiobridgeserver/internal/BridgeServer.h>
#include <audiobridgeserver/internal/BridgeStatusAddOn.h>
#include <audiobridgeserver/internal/DownloadBridgeAddOn.h>

#include <org.diffscope.audiobridgeserver_actions.qak.h>

namespace AudioBridge::Internal {

    bool AudioBridgeServerPlugin::initialize(const QStringList &, QString *) {
        m_server = new BridgeServer(this);
        return true;
    }

    void AudioBridgeServerPlugin::extensionsInitialized() {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::audiobridgeserverActionExtension());
        Core::HomeWindowInterfaceRegistry::instance()->attach<DownloadBridgeAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<DownloadBridgeAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<BridgeStatusAddOn>();
        m_server->start();
    }

    ExtensionSystem::IPlugin::ShutdownFlag AudioBridgeServerPlugin::aboutToShutdown() {
        m_server->stop();
        return SynchronousShutdown;
    }

}
