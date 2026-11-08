// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "SourcesPicker.h"

#include <memory>

#include <QEventLoop>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/SourcesPickerModel.h>

namespace Core {

    std::optional<DspxProject::SourcesSelection> pickSources(const DspxProject::SourcesSelection &initial, QQuickWindow *window) {
        if (!window)
            return std::nullopt;

        SourcesPickerModel draft;
        draft.setSourcesSelection(initial);
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "SourcesPickerDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        QObject *object = component.createWithInitialProperties({
            {"sourcesModel", QVariant::fromValue(&draft)},
        });
        if (!object) {
            qFatal() << component.errorString();
        }
        const std::unique_ptr<QObject> objectOwner(object);
        auto *dialog = qobject_cast<QQuickWindow *>(object);
        if (!dialog) {
            qFatal("SourcesPickerDialog in DiffScope.Core is not QQuickWindow");
        }

        const QPointer<QQuickWindow> dialogWindow(window);
        dialog->setTransientParent(window);
        QEventLoop loop;
        QObject::connect(dialog, SIGNAL(finished()), &loop, SLOT(quit()));
        QObject::connect(window, &QObject::destroyed, &loop, &QEventLoop::quit);
        dialog->show();
        if (dialogWindow && dialog->isVisible()) {
            loop.exec();
        }
        if (!dialogWindow || !dialog->property("acceptedResult").toBool())
            return std::nullopt;
        return draft.sourcesSelection();
    }

}
