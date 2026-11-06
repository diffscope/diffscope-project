// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreEditSourcesStrategy.h"

#include <memory>

#include <QEventLoop>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/SourcesPickerModel.h>

namespace Core::Internal {

    CoreEditSourcesStrategy::CoreEditSourcesStrategy(DspxProject::Document *document) : m_document(document) {
    }

    std::optional<DspxProject::SourcesSelection> CoreEditSourcesStrategy::requestSources(const DspxProject::SourcesSelection &initial, QQuickWindow *window, DspxProject::DocumentEdit::Hints) {
        if (!window) {
            return std::nullopt;
        }

        SourcesPickerModel draft;
        draft.setArchitectureId(initial.architectureId);
        draft.setSingers(initial.singers);
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "SourcesPickerDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QObject> object(component.createWithInitialProperties({
            {"sourcesModel", QVariant::fromValue(&draft)},
        }));
        if (!object) {
            qFatal() << component.errorString();
        }
        auto dialog = qobject_cast<QQuickWindow *>(object.get());
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
        if (!dialogWindow || !dialog->property("acceptedResult").toBool()) {
            return std::nullopt;
        }
        return draft.sourcesSelection();
    }

    void CoreEditSourcesStrategy::reportUnsupportedNoteLanguages(QQuickWindow *window) {
        SVS::MessageBox::warning(
            RuntimeInterface::qmlEngine(), window,
            tr("Unsupported Note Languages"),
            tr("Some notes use languages that the selected singer does not support. Synthesis may fail. Change the note languages to languages supported by the singer.")
        );
    }

    void CoreEditSourcesStrategy::reportOperationConflict() {
        ProjectWindowInterface::of(m_document)->sendOperationConflictNotification();
    }

}
