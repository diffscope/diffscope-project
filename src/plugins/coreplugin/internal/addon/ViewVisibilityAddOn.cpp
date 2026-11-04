// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ViewVisibilityAddOn.h"

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {
    ViewVisibilityAddOn::ViewVisibilityAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }
    ViewVisibilityAddOn::~ViewVisibilityAddOn() = default;

    Q_STATIC_LOGGING_CATEGORY(lcViewVisibilityAddOn, "diffscope.core.viewvisibilityaddon")

    void ViewVisibilityAddOn::initialize() {
        auto state = RuntimeInterface::state();
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto window = windowInterface->window();

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ViewVisibilityAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        if (!o) {
            qFatal() << component.errorString();
        }
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());

        qCDebug(lcViewVisibilityAddOn) << "Loading view visibility properties from settings";

        state->beginGroup(QStringLiteral("org.diffscope.core.viewVisibility"));

        auto menuBarVisible = !state->value("menuBarHidden", false).toBool();
        qCDebug(lcViewVisibilityAddOn) << "Menu bar" << menuBarVisible;
        auto menuBar = window->property("menuBar").value<QObject *>();
        menuBar->setProperty("alwaysVisible", menuBarVisible);

        auto toolBarVisible = !state->value("toolBarHidden", false).toBool();
        qCDebug(lcViewVisibilityAddOn) << "Tool bar" << toolBarVisible;
        auto toolBar = window->property("toolBar").value<QObject *>();
        toolBar->setProperty("visible", toolBarVisible);

        auto statusBarVisible = !state->value("statusBarHidden", false).toBool();
        qCDebug(lcViewVisibilityAddOn) << "Status bar" << statusBarVisible;
        auto statusBar = window->property("statusBar").value<QObject *>();
        statusBar->setProperty("visible", statusBarVisible);

        state->endGroup();
    }
    void ViewVisibilityAddOn::extensionsInitialized() {
    }
    bool ViewVisibilityAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    void ViewVisibilityAddOn::toggleVisibility(ViewVisibilityOption option, bool visible) const {
        auto state = RuntimeInterface::state();
        auto window = windowHandle()->window();
        qCInfo(lcViewVisibilityAddOn) << "Toggling visibility started";
        state->beginGroup(QStringLiteral("org.diffscope.core.viewVisibility"));
        if (option == MenuBar) {
            auto menuBar = window->property("menuBar").value<QObject *>();
            qCInfo(lcViewVisibilityAddOn) << "Menu bar" << visible;
            menuBar->setProperty("alwaysVisible", visible);
        } else if (option == ToolBar) {
            auto toolBar = window->property("toolBar").value<QObject *>();
            qCInfo(lcViewVisibilityAddOn) << "Tool bar" << visible;
            toolBar->setProperty("visible", visible);
        } else if (option == StatusBar) {
            auto statusBar = window->property("statusBar").value<QObject *>();
            qCInfo(lcViewVisibilityAddOn) << "Status bar" << visible;
            statusBar->setProperty("visible", visible);
        }
        const char *keys[]{"menuBarHidden", "toolBarHidden", "statusBarHidden"};
        if (option >= MenuBar && option <= StatusBar) state->setValue(keys[option], !visible);
        state->endGroup();
    }
}
