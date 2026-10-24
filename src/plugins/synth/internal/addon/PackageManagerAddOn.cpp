// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageManagerAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ActionWindowInterfaceBase.h>

#include <synth/internal/PackageManagerController.h>

namespace Synth::Internal {

    PackageManagerAddOn::PackageManagerAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent), m_packageManager(new PackageManagerController(this)) {
    }

    PackageManagerAddOn::~PackageManagerAddOn() = default;

    void PackageManagerAddOn::openPackageManager() {
        m_packageManager->openPackageManager(windowHandle()->window());
    }

    void PackageManagerAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Synth", "PackageManagerActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
        if (!object) {
            qFatal() << component.errorString();
        }
        object->setParent(this);
        QMetaObject::invokeMethod(object, "registerToContext", windowInterface->actionContext());
    }

    void PackageManagerAddOn::extensionsInitialized() {
    }

    bool PackageManagerAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}
