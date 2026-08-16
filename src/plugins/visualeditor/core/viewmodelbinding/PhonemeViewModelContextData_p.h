// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_PHONEMEVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_PHONEMEVIEWMODELCONTEXTDATA_P_H

#include <limits>
#include <map>
#include <utility>

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPair>
#include <QPointer>
#include <QSet>
#include <QString>

#include <interval-tree/interval_tree.hpp>

#include <transactional/TransactionController.h>
#include <visualeditor/ProjectViewModelContext.h>

class QQuickItem;

namespace dspx {
    class Note;
    class Phoneme;
    class PhonemeSequence;
    class SingingClip;
    class Tempo;
    class TempoSequence;
    class Track;
    class TrackList;
}

namespace sflow {
    class PhonemeSequenceInteractionController;
    class PhonemeViewModel;
    class PointSequenceViewModel;
}

namespace SVS {
    class MusicTimeline;
}

namespace Core {
    class DspxDocument;
}

namespace VisualEditor {

    class PhonemeViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        struct NoteInterval : lib_interval_tree::interval<int> {
            NoteInterval();
            NoteInterval(int start, int end, dspx::Note *note);

            dspx::Note *note() const;
            bool operator==(const NoteInterval &other) const;

        private:
            dspx::Note *m_note{};
        };

        using OrderKey = std::pair<double, quint64>;

        struct NoteBinding {
            dspx::Note *note{};
            dspx::SingingClip *clip{};
            dspx::PhonemeSequence *activeSequence{};
            bool edited{};
            quint64 nextStableOrder{};
            QList<sflow::PhonemeViewModel *> viewItems;
            QHash<sflow::PhonemeViewModel *, quint64> stableOrders;
            QHash<sflow::PhonemeViewModel *, OrderKey> orderKeys;
            std::map<OrderKey, sflow::PhonemeViewModel *> order;
        };

        enum EditOperation {
            NoEdit,
            MoveEdit,
            TokenEdit,
        };

        ProjectViewModelContext *q_ptr{};

        ~PhonemeViewModelContextData() override;

        Core::DspxDocument *document{};
        SVS::MusicTimeline *musicTimeline{};
        dspx::TempoSequence *tempoSequence{};
        dspx::TrackList *trackList{};

        QSet<dspx::Track *> boundTracks;
        QHash<dspx::SingingClip *, sflow::PointSequenceViewModel *> phonemeSequenceViewModelMap;
        QHash<dspx::SingingClip *, QSet<dspx::Note *>> clipNotes;
        QHash<dspx::Note *, NoteBinding *> noteBindings;
        QHash<dspx::Phoneme *, sflow::PhonemeViewModel *> phonemeViewItemMap;
        QHash<sflow::PhonemeViewModel *, dspx::Phoneme *> phonemeDocumentItemMap;

        QHash<dspx::Note *, NoteInterval> noteIntervals;
        lib_interval_tree::interval_tree<NoteInterval> noteIntervalTree;

        std::map<int, dspx::Tempo *> tempoItems;
        QHash<dspx::Tempo *, int> tempoPositions;
        QList<QPair<int, int>> dirtyTempoRanges;
        QSet<dspx::Note *> dirtyNoteRebuilds;
        QSet<dspx::Note *> dirtyNotes;
        QSet<dspx::SingingClip *> dirtyClips;
        bool updateScheduled{};

        int syncingViewDepth{};
        bool committingDocumentChanges{};
        EditOperation editOperation{NoEdit};
        QPointer<sflow::PhonemeViewModel> targetViewItem;
        QPointer<QQuickItem> targetSequenceItem;
        QMetaObject::Connection targetSequenceDestroyedConnection;
        double originalViewPosition{};
        QString originalViewContent;
        Core::TransactionController::TransactionId transactionId{
            Core::TransactionController::TransactionId::Invalid
        };

        void init();
        void bindTrackSequences();
        void bindTrack(dspx::Track *track);
        void unbindTrack(dspx::Track *track);
        void bindSingingClip(dspx::SingingClip *clip);
        void unbindSingingClip(dspx::SingingClip *clip);
        void bindNote(dspx::Note *note);
        void unbindNote(dspx::Note *note);
        void rebuildNote(dspx::Note *note);
        void clearActivePhonemes(NoteBinding *binding);
        void bindActivePhoneme(NoteBinding *binding, dspx::Phoneme *phoneme);
        void connectActivePhoneme(dspx::Phoneme *phoneme,
                                  sflow::PhonemeViewModel *viewItem,
                                  bool connectViewItem = true);

        void insertIntoLiveOrder(NoteBinding *binding, sflow::PhonemeViewModel *viewItem);
        void removeFromLiveOrder(NoteBinding *binding, sflow::PhonemeViewModel *viewItem);
        void updateLiveOrder(NoteBinding *binding, sflow::PhonemeViewModel *viewItem);
        void rebuildLiveOrder(NoteBinding *binding);

        void bindTempo(dspx::Tempo *tempo);
        void unbindTempo(dspx::Tempo *tempo);
        void handleTempoPositionChanged(dspx::Tempo *tempo);
        void addDirtyTempoRange(int start, int end);
        int tempoSegmentEnd(int position) const;

        void scheduleNoteRebuild(dspx::Note *note);
        void scheduleNoteUpdate(dspx::Note *note);
        void scheduleClipUpdate(dspx::SingingClip *clip);
        void schedulePendingUpdate();
        void flushPendingUpdates();
        void updateNoteInterval(NoteBinding *binding);
        void removeNoteInterval(dspx::Note *note);
        void syncNotePositions(NoteBinding *binding);

        int noteAbsoluteTick(const dspx::Note *note) const;
        double tickToMilliseconds(double tick) const;
        double millisecondsToTick(double milliseconds) const;
        double documentPositionToViewPosition(const dspx::Note *note, int milliseconds) const;
        int viewPositionToDocumentPosition(const dspx::Note *note, double position) const;

        NoteBinding *bindingForViewItem(sflow::PhonemeViewModel *viewItem) const;
        void handleViewPositionChanged(sflow::PhonemeViewModel *viewItem);
        void handleViewContentChanged(sflow::PhonemeViewModel *viewItem);

        bool beginEdit(EditOperation operation, QQuickItem *sequenceItem,
                       sflow::PhonemeViewModel *viewItem);
        void commitMove();
        void commitTokenEdit();
        void abortEdit();
        void finishEdit();
        dspx::Phoneme *ensureEditedPhonemes(NoteBinding *binding,
                                            sflow::PhonemeViewModel *target);
        void restoreTargetViewItem();

        sflow::PhonemeSequenceInteractionController *createController(QObject *parent);
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_PHONEMEVIEWMODELCONTEXTDATA_P_H
