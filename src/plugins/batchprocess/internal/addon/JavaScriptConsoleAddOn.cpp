// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JavaScriptConsoleAddOn.h"

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QWindow>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/ActionWindowInterfaceBase.h>

#include <batchprocess/JavaScriptConsoleInterface.h>
#include <batchprocess/internal/JavaScriptDebugConsoleDialog.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcJavaScriptConsoleAddOn, "diffscope.batchprocess.javascriptconsole.addon")

    JavaScriptConsoleAddOn::JavaScriptConsoleAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    JavaScriptConsoleAddOn::~JavaScriptConsoleAddOn() {
        delete m_dialog;
    }

    void JavaScriptConsoleAddOn::showConsole() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        if (!m_dialog) {
            m_dialog = new JavaScriptDebugConsoleDialog(JavaScriptConsoleInterface::instance()->model());
            m_dialog->setAttribute(Qt::WA_NativeWindow);
            (void)m_dialog->winId();
            if (auto dialogWindow = m_dialog->windowHandle()) {
                dialogWindow->setTransientParent(windowInterface->window());
            }
            qCDebug(lcJavaScriptConsoleAddOn) << "Created JavaScript debug console dialog for" << windowInterface;
        }
        qCInfo(lcJavaScriptConsoleAddOn) << "Showing JavaScript debug console" << windowInterface;
        m_dialog->show();
        m_dialog->raise();
        m_dialog->activateWindow();
    }

    void JavaScriptConsoleAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.BatchProcess", "JavaScriptConsoleActions");
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

    void JavaScriptConsoleAddOn::extensionsInitialized() {
    }

    bool JavaScriptConsoleAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}

#include "moc_JavaScriptConsoleAddOn.cpp"
