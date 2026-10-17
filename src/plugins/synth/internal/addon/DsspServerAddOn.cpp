// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DsspServerAddOn.h"

#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ActionWindowInterfaceBase.h>

#include <synth/internal/DsspServerManager.h>

namespace Synth::Internal {

    DsspServerAddOn::DsspServerAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    DsspServerAddOn::~DsspServerAddOn() = default;

    void DsspServerAddOn::initialize() {
        // Action components must be registered before the window's initial layout update.
        auto window = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        auto server = Core::RuntimeInterface::instance()->getFirstObject<DsspServerManager>();
        Q_ASSERT(server);
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("DsspServerActions"));
        if (component.isError())
            qFatal() << component.errorString();
        auto actions = component.createWithInitialProperties({
            {QStringLiteral("server"), QVariant::fromValue(server)},
            {QStringLiteral("windowHandle"), QVariant::fromValue(window)},
        });
        if (!actions)
            qFatal() << component.errorString();
        actions->setParent(this);
        QMetaObject::invokeMethod(actions, "registerToContext", window->actionContext());
    }

    void DsspServerAddOn::extensionsInitialized() {
    }

}

#include "moc_DsspServerAddOn.cpp"
