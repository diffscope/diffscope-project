// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DocumentEditDialog.h"

#include <QEventLoop>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

namespace Core::Internal::DocumentEditDialog {

    UIShell::DialogPositioner::Placement placementFor(DspxProject::DocumentEdit::Hints hints) {
        using Placement = UIShell::DialogPositioner::Placement;
        return hints.testFlag(DspxProject::DocumentEdit::AtCursor) ? Placement::AtCursor : Placement::WindowDefault;
    }

    std::unique_ptr<QObject> createPopup(const char *type, QQuickWindow *window, UIShell::DialogPositioner::Placement placement, const QVariantMap &initialProperties) {
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", type);
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto properties = initialProperties;
        properties.insert(QStringLiteral("parent"), QVariant::fromValue(window->contentItem()));
        std::unique_ptr<QObject> popup(component.createWithInitialProperties(properties));
        if (!popup) {
            qFatal() << component.errorString();
        }
        UIShell::DialogPositioner::positionPopup(popup.get(), window, placement);
        return popup;
    }

    bool execPopup(QObject *popup, QQuickWindow *window) {
        QEventLoop loop;
        const QPointer<QQuickWindow> dialogWindow(window);
        QObject::connect(popup, SIGNAL(accepted()), &loop, SLOT(quit()));
        QObject::connect(popup, SIGNAL(rejected()), &loop, SLOT(quit()));
        QObject::connect(popup, SIGNAL(closed()), &loop, SLOT(quit()));
        QObject::connect(window, &QObject::destroyed, &loop, &QEventLoop::quit);
        QMetaObject::invokeMethod(popup, "open");
        if (dialogWindow && popup->property("visible").toBool()) {
            loop.exec();
        }
        return dialogWindow && popup->property("result").toInt() == 1;
    }

}
