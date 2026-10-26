// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "FileDropAddOn.h"

#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

namespace Core::Internal {
    FileDropAddOn::FileDropAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }
    FileDropAddOn::~FileDropAddOn() = default;
    void FileDropAddOn::initialize() {
        auto window = qobject_cast<QQuickWindow *>(windowHandle()->window());
        if (!window) {
            qFatal("FileDropAddOn requires a QQuickWindow");
        }
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "FileDropArea");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({
            {"parent", QVariant::fromValue(window->contentItem())},
        });
        if (!object) {
            qFatal() << component.errorString();
        }
        if (!qobject_cast<QQuickItem *>(object)) {
            qFatal("FileDropArea in DiffScope.Core is not QQuickItem");
        }
    }
    void FileDropAddOn::extensionsInitialized() {
    }
    bool FileDropAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}
