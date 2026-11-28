// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DownloadBridgeAddOn.h"

#include <QDesktopServices>
#include <QMetaObject>
#include <QQmlComponent>
#include <QUrl>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ActionWindowInterfaceBase.h>

namespace AudioBridge::Internal {

    static const QUrl DOWNLOAD_URL(QStringLiteral("https://diffscope.org/downloads/#diffscope-bridge"));

    DownloadBridgeAddOn::DownloadBridgeAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    DownloadBridgeAddOn::~DownloadBridgeAddOn() = default;

    void DownloadBridgeAddOn::initialize() {
        auto *windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.AudioBridgeServer", "DownloadBridgeAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto *actions = component.createWithInitialProperties({{QStringLiteral("addOn"), QVariant::fromValue(this)}});
        if (!actions) {
            qFatal() << component.errorString();
        }
        actions->setParent(this);
        QMetaObject::invokeMethod(actions, "registerToContext", windowInterface->actionContext());
    }

    void DownloadBridgeAddOn::extensionsInitialized() {
    }

    void DownloadBridgeAddOn::openDownloadPage() {
        QDesktopServices::openUrl(DOWNLOAD_URL);
    }

}
