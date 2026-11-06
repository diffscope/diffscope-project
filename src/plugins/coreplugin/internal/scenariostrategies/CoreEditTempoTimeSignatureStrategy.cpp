// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreEditTempoTimeSignatureStrategy.h"

#include <SVSCraftCore/MusicTimeline.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/DocumentEditDialog.h>

namespace Core::Internal {

    CoreEditTempoTimeSignatureStrategy::CoreEditTempoTimeSignatureStrategy(DspxProject::Document *document) : m_document(document) {
    }

    std::optional<DspxProject::EditTempoTimeSignatureStrategy::TempoInput> CoreEditTempoTimeSignatureStrategy::requestTempo(const TempoInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("EditTempoDialog", window, DocumentEditDialog::placementFor(hints), {
            {"timeline", QVariant::fromValue(timeline)},
            {"position", initial.position},
            {"doInsertNew", initial.insert},
        });
        dialog->setProperty("tempo", initial.tempo);
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return TempoInput{dialog->property("position").toInt(), dialog->property("doInsertNew").toBool(), dialog->property("tempo").toDouble()};
    }

    std::optional<DspxProject::EditTempoTimeSignatureStrategy::TimeSignatureInput> CoreEditTempoTimeSignatureStrategy::requestTimeSignature(const TimeSignatureInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("EditTimeSignatureDialog", window, DocumentEditDialog::placementFor(hints), {
            {"timeline", QVariant::fromValue(timeline)},
            {"position", initial.position},
            {"doInsertNew", initial.insert},
        });
        dialog->setProperty("numerator", initial.numerator);
        dialog->setProperty("denominator", initial.denominator);
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return TimeSignatureInput{dialog->property("position").toInt(), dialog->property("doInsertNew").toBool(),
                                  dialog->property("numerator").toInt(), dialog->property("denominator").toInt()};
    }

    void CoreEditTempoTimeSignatureStrategy::reportOperationConflict() {
        ProjectWindowInterface::of(m_document)->sendOperationConflictNotification();
    }

}
