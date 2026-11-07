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

    /** Supplies item insertion interaction. Window arguments are optional dialog parents. */
    class DSPXPROJECT_EXPORT InsertItemStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit InsertItemStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~InsertItemStrategy() override;

        /** The initial or accepted track insertion values. */
        struct TrackInput {
            /** The zero-based insertion index. */
            int index;
            /** The number of tracks to insert. A nonpositive count performs no edit. */
            int count;
            /** The base track name. Additional tracks receive numbered suffixes. */
            QString name;
        };

        /** The initial or accepted label values. */
        struct LabelInput {
            /** The position in project ticks. */
            int position;
            /** The label text. */
            QString text;
        };

        /** The initial or accepted singing clip values. */
        struct ClipInput {
            /** The borrowed destination track, or nullptr to retain the initial track. */
            dspx::Track *track;
            /** The position in project ticks. */
            int position;
            /** The visible clip length in ticks. */
            int length;
            /** The clip name. */
            QString name;
        };

        /** The initial or accepted note values. */
        struct NoteInput {
            /** The position in clip content ticks. */
            int position;
            /** The note length in ticks. */
            int length;
            /** The MIDI pitch number. */
            int pitch;
            /** The lyric assigned to the note. */
            QString lyric;
        };

        /** The accepted edited phoneme values. */
        struct PhonemeInput {
            /** The phoneme token. */
            QString token;
            /** The phoneme start relative to the note in milliseconds. */
            int start;
            /** The phoneme language identifier. */
            QString language;
            /** Whether the phoneme is an onset. */
            bool onset;
        };

        /** Returns accepted track values, or nullopt on cancellation. */
        virtual std::optional<TrackInput> requestTrack(int trackCount, const TrackInput &initial, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Returns accepted label values, or nullopt on cancellation. @pre timeline is non-null. */
        virtual std::optional<LabelInput> requestLabel(const LabelInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /**
         * Returns accepted clip values, or nullopt on cancellation.
         * @pre tracks and timeline are non-null. initial.track is a track in tracks.
         * @post An accepted track is nullptr or a member of tracks.
         */
        virtual std::optional<ClipInput> requestClip(dspx::TrackList *tracks, const ClipInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Returns accepted note values, or nullopt on cancellation. @pre timeline is non-null. */
        virtual std::optional<NoteInput> requestNote(const NoteInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Returns accepted phoneme values, or nullopt on cancellation. */
        virtual std::optional<PhonemeInput> requestPhoneme(QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Reports a conflicting operation that prevents a transaction. */
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_INSERTITEMSTRATEGY_H
