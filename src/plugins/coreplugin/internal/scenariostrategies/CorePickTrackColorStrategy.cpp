// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CorePickTrackColorStrategy.h"

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/DocumentEditDialog.h>

namespace Core::Internal {

    CorePickTrackColorStrategy::CorePickTrackColorStrategy(DspxProject::Document *document) : m_document(document) {
    }

    std::optional<int> CorePickTrackColorStrategy::requestColor(int currentColorId, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("PickTrackColorDialog", window, DocumentEditDialog::placementFor(hints), {
            {"colorId", currentColorId},
        });
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return dialog->property("colorId").toInt();
    }

    void CorePickTrackColorStrategy::reportOperationConflict() {
        ProjectWindowInterface::of(m_document)->sendOperationConflictNotification();
    }

}
