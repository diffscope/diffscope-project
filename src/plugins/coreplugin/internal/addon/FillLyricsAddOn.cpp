// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "FillLyricsAddOn.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    FillLyricsAddOn::FillLyricsAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    FillLyricsAddOn::~FillLyricsAddOn() = default;

    void FillLyricsAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "FillLyricsAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto actions = component.createWithInitialProperties({
            {"windowHandle", QVariant::fromValue(windowInterface)},
        });
        if (!actions) {
            qFatal() << component.errorString();
        }
        actions->setParent(this);
        QMetaObject::invokeMethod(actions, "registerToContext", windowInterface->actionContext());
    }
    void FillLyricsAddOn::extensionsInitialized() {
    }

}
