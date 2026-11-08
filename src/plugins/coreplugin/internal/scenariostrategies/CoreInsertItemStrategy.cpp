// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreInsertItemStrategy.h"

#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/SingerSettings.h>
#include <coreplugin/internal/DocumentEditDialog.h>

namespace Core::Internal {

    CoreInsertItemStrategy::CoreInsertItemStrategy(DspxProject::Document *document) : m_document(document) {
    }

    std::optional<DspxProject::InsertItemStrategy::TrackInput> CoreInsertItemStrategy::requestTrack(int trackCount, const TrackInput &initial, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("InsertTrackDialog", window, DocumentEditDialog::placementFor(hints), {
            {"trackCount", trackCount},
            {"insertionIndex", initial.index},
            {"insertionCount", initial.count},
            {"trackName", initial.name},
        });
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return TrackInput{dialog->property("insertionIndex").toInt(), dialog->property("insertionCount").toInt(), dialog->property("trackName").toString()};
    }

    std::optional<DspxProject::InsertItemStrategy::LabelInput> CoreInsertItemStrategy::requestLabel(const LabelInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("InsertLabelDialog", window, DocumentEditDialog::placementFor(hints), {
            {"timeline", QVariant::fromValue(timeline)},
            {"labelPos", initial.position},
            {"labelText", initial.text},
        });
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return LabelInput{dialog->property("labelPos").toInt(), dialog->property("labelText").toString()};
    }

    std::optional<DspxProject::InsertItemStrategy::ClipInput> CoreInsertItemStrategy::requestClip(dspx::TrackList *tracks, const ClipInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("InsertSingingClipDialog", window, DocumentEditDialog::placementFor(hints), {
            {"trackList", QVariant::fromValue(tracks)},
            {"selectedTrack", QVariant::fromValue(initial.track)},
            {"timeline", QVariant::fromValue(timeline)},
            {"clipPosition", initial.position},
            {"clipLength", initial.length},
            {"clipName", initial.name},
        });
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return ClipInput{
            .track = qobject_cast<dspx::Track *>(dialog->property("selectedTrack").value<QObject *>()),
            .position = dialog->property("clipPosition").toInt(),
            .length = dialog->property("clipLength").toInt(),
            .name = dialog->property("clipName").toString(),
            .sources = SingerSettings::defaultSinger(),
        };
    }

    std::optional<DspxProject::InsertItemStrategy::NoteInput> CoreInsertItemStrategy::requestNote(const NoteInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("InsertNoteDialog", window, DocumentEditDialog::placementFor(hints), {
            {"timeline", QVariant::fromValue(timeline)},
            {"notePosition", initial.position},
            {"noteLength", initial.length},
            {"notePitch", initial.pitch},
            {"noteLyric", initial.lyric},
        });
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return NoteInput{dialog->property("notePosition").toInt(), dialog->property("noteLength").toInt(),
                     dialog->property("notePitch").toInt(), dialog->property("noteLyric").toString()};
    }

    std::optional<DspxProject::InsertItemStrategy::PhonemeInput> CoreInsertItemStrategy::requestPhoneme(QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) {
        if (!window) {
            return std::nullopt;
        }
        auto dialog = DocumentEditDialog::createPopup("InsertPhonemeDialog", window, DocumentEditDialog::placementFor(hints), {
            {"token", QString()},
            {"start", 0},
            {"language", QString()},
            {"onset", false},
        });
        if (!DocumentEditDialog::execPopup(dialog.get(), window)) {
            return std::nullopt;
        }
        return PhonemeInput{dialog->property("token").toString(), dialog->property("start").toInt(),
                        dialog->property("language").toString(), dialog->property("onset").toBool()};
    }

    void CoreInsertItemStrategy::reportOperationConflict() {
        ProjectWindowInterface::of(m_document)->sendOperationConflictNotification();
    }

}
