// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioBridgeServerPlugin.h"

#include <QMetaObject>
#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>
#include <CoreApi/windowinterface.h>

#include <extensionsystem/pluginspec.h>
#include <QAKCore/actionregistry.h>
#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audiobridgeserver/internal/BridgeServer.h>

#include <org.diffscope.audiobridgeserver_actions.qak.h>

namespace AudioBridge::Internal {

    namespace {

        class BridgeStatusAddOn : public Core::WindowInterfaceAddOn {
        public:
            using WindowInterfaceAddOn::WindowInterfaceAddOn;

            void initialize() override {
                auto *projectWindow = windowHandle()->cast<Core::ProjectWindowInterface>();
                QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.AudioBridgeServer", "BridgeStatusActions");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto *actions = component.createWithInitialProperties({{QStringLiteral("windowHandle"), QVariant::fromValue(projectWindow)}});
                if (!actions) {
                    qFatal() << component.errorString();
                }
                actions->setParent(this);
                QMetaObject::invokeMethod(actions, "registerToContext", projectWindow->actionContext());
            }

            void extensionsInitialized() override {
            }
        };

    }

    bool AudioBridgeServerPlugin::initialize(const QStringList &, QString *) {
        m_server = new BridgeServer(this);
        return true;
    }

    void AudioBridgeServerPlugin::extensionsInitialized() {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::audiobridgeserverActionExtension());
        Core::ProjectWindowInterfaceRegistry::instance()->attach<BridgeStatusAddOn>();
        m_server->start();
    }

    ExtensionSystem::IPlugin::ShutdownFlag AudioBridgeServerPlugin::aboutToShutdown() {
        m_server->stop();
        return SynchronousShutdown;
    }

}
