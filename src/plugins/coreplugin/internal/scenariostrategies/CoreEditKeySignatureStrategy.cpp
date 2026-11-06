// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreEditKeySignatureStrategy.h"

#include <SVSCraftCore/MusicTimeline.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/DocumentEditDialog.h>

namespace Core::Internal {

    CoreEditKeySignatureStrategy::CoreEditKeySignatureStrategy(DspxProject::Document *document) : m_document(document) {
    }

    std::optional<DspxProject::EditKeySignatureStrategy::Input> CoreEditKeySignatureStrategy::requestKeySignature(const Input &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("EditKeySignatureDialog", window, DocumentEditDialog::placementFor(hints), {
            {"timeline", QVariant::fromValue(timeline)},
            {"position", initial.position},
            {"doInsertNew", initial.insert},
        });
        dialog->setProperty("tonality", initial.tonality);
        dialog->setProperty("mode", initial.mode);
        dialog->setProperty("accidentalType", initial.accidentalType);
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return Input{dialog->property("position").toInt(), dialog->property("doInsertNew").toBool(),
                     dialog->property("tonality").toInt(), dialog->property("mode").toInt(), dialog->property("accidentalType").toInt()};
    }

    void CoreEditKeySignatureStrategy::reportOperationConflict() {
        ProjectWindowInterface::of(m_document)->sendOperationConflictNotification();
    }

}
