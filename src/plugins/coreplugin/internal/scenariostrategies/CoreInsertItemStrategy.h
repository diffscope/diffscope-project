// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREINSERTITEMSTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREINSERTITEMSTRATEGY_H

#include <dspxproject/InsertItemStrategy.h>

namespace DspxProject {
    class Document;
}

namespace Core::Internal {

    class CoreInsertItemStrategy : public DspxProject::InsertItemStrategy {
    public:
        explicit CoreInsertItemStrategy(DspxProject::Document *document);

        std::optional<TrackInput> requestTrack(int trackCount, const TrackInput &initial, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        std::optional<LabelInput> requestLabel(const LabelInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        std::optional<ClipInput> requestClip(dspx::TrackList *tracks, const ClipInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        std::optional<NoteInput> requestNote(const NoteInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        std::optional<PhonemeInput> requestPhoneme(QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        void reportOperationConflict() override;

    private:
        DspxProject::Document *const m_document;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREINSERTITEMSTRATEGY_H
