// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_INSERTITEMSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_INSERTITEMSTRATEGY_H

#include <optional>

#include <QObject>
#include <QString>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace SVS {
    class MusicTimeline;
}

namespace dspx {
    class Track;
    class TrackList;
}

namespace DspxProject {

    class DSPXPROJECT_EXPORT InsertItemStrategy : public QObject {
        Q_OBJECT
    public:
        explicit InsertItemStrategy(QObject *parent = nullptr);
        ~InsertItemStrategy() override;

        struct TrackInput {
            int index;
            int count;
            QString name;
        };

        struct LabelInput {
            int position;
            QString text;
        };

        struct ClipInput {
            dspx::Track *track;
            int position;
            int length;
            QString name;
        };

        struct NoteInput {
            int position;
            int length;
            int pitch;
            QString lyric;
        };

        struct PhonemeInput {
            QString token;
            int start;
            QString language;
            bool onset;
        };

        virtual std::optional<TrackInput> requestTrack(int trackCount, const TrackInput &initial, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual std::optional<LabelInput> requestLabel(const LabelInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual std::optional<ClipInput> requestClip(dspx::TrackList *tracks, const ClipInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual std::optional<NoteInput> requestNote(const NoteInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual std::optional<PhonemeInput> requestPhoneme(QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_INSERTITEMSTRATEGY_H
