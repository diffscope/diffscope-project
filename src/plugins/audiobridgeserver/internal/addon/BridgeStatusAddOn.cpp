// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BridgeStatusAddOn.h"

#include <QMetaObject>
#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace AudioBridge::Internal {

    BridgeStatusAddOn::BridgeStatusAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    BridgeStatusAddOn::~BridgeStatusAddOn() = default;

    void BridgeStatusAddOn::initialize() {
        auto *projectWindow = windowHandle()->cast<Core::ProjectWindowInterface>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.AudioBridgeServer", "BridgeStatusAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto *actions = component.createWithInitialProperties({{QStringLiteral("addOn"), QVariant::fromValue(this)}});
        if (!actions) {
            qFatal() << component.errorString();
        }
        actions->setParent(this);
        QMetaObject::invokeMethod(actions, "registerToContext", projectWindow->actionContext());
    }

    void BridgeStatusAddOn::extensionsInitialized() {
    }

}
