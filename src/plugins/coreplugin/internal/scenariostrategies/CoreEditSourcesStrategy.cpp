// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreEditSourcesStrategy.h"

#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/SourcesPicker.h>

namespace Core::Internal {

    CoreEditSourcesStrategy::CoreEditSourcesStrategy(DspxProject::Document *document) : m_document(document) {
    }

    std::optional<DspxProject::SourcesSelection> CoreEditSourcesStrategy::requestSources(const DspxProject::SourcesSelection &initial, QQuickWindow *window, DspxProject::DocumentEdit::Hints) {
        return pickSources(initial, window);
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
