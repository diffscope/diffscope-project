// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ItemSelectorAddOn.h"

#include <QQmlComponent>
#include <QWindow>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/ItemSelectorDialog.h>

namespace Core::Internal {

    ItemSelectorAddOn::ItemSelectorAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ItemSelectorAddOn::~ItemSelectorAddOn() {
        delete m_dialog;
    }

    void ItemSelectorAddOn::initialize() {
        auto *windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ItemSelectorAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto *object = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        object->setParent(this);
        QMetaObject::invokeMethod(object, "registerToContext", windowInterface->actionContext());
    }

    void ItemSelectorAddOn::extensionsInitialized() {
    }

    bool ItemSelectorAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    void ItemSelectorAddOn::showItemSelector() {
        auto *windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        if (!m_dialog) {
            m_dialog = new ItemSelectorDialog(windowInterface);
            m_dialog->setAttribute(Qt::WA_NativeWindow);
            (void)m_dialog->winId();
            if (auto *dialogWindow = m_dialog->windowHandle()) {
                dialogWindow->setTransientParent(windowInterface->window());
            }
        }

        m_dialog->show();
        m_dialog->raise();
        m_dialog->activateWindow();
    }

}
