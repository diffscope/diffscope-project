// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreEditLoopStrategy.h"

#include <SVSCraftCore/MusicTimeline.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/DocumentEditDialog.h>

namespace Core::Internal {

    CoreEditLoopStrategy::CoreEditLoopStrategy(DspxProject::Document *document) : m_document(document) {
    }

    std::optional<DspxProject::EditLoopStrategy::Input> CoreEditLoopStrategy::requestLoop(const Input &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("EditLoopDialog", window, DocumentEditDialog::placementFor(hints), {
            {"timeline", QVariant::fromValue(timeline)},
            {"loopEnabled", initial.enabled},
            {"startPosition", initial.start},
            {"endPosition", initial.end},
        });
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return Input{dialog->property("loopEnabled").toBool(), dialog->property("startPosition").toInt(), dialog->property("endPosition").toInt()};
    }

    void CoreEditLoopStrategy::reportOperationConflict() {
        ProjectWindowInterface::of(m_document)->sendOperationConflictNotification();
    }

}
