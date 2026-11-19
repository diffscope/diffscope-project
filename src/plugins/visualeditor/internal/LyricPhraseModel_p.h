// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_LYRICPHRASEMODEL_P_H
#define DIFFSCOPE_VISUALEDITOR_LYRICPHRASEMODEL_P_H

#include <map>
#include <set>
#include <unordered_map>
#include <utility>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <qqmlintegration.h>

#include <ScopicFlowCore/NoteViewModel.h>
#include <ScopicFlowCore/RangeSequenceViewModel.h>
#include <ScopicFlowCore/SelectionController.h>

namespace VisualEditor::Internal {

    class LyricPhraseModel : public QAbstractListModel {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(sflow::RangeSequenceViewModel *noteSequence READ noteSequence WRITE setNoteSequence NOTIFY noteSequenceChanged)
        Q_PROPERTY(int gapThreshold READ gapThreshold WRITE setGapThreshold NOTIFY gapThresholdChanged)
        Q_PROPERTY(QRectF visibleRange READ visibleRange WRITE setVisibleRange NOTIFY visibleRangeChanged)
        Q_PROPERTY(qreal minimumDuration READ minimumDuration WRITE setMinimumDuration NOTIFY minimumDurationChanged)

    public:
        explicit LyricPhraseModel(QObject *parent = nullptr);

        sflow::RangeSequenceViewModel *noteSequence() const;
        void setNoteSequence(sflow::RangeSequenceViewModel *noteSequence);
        int gapThreshold() const;
        void setGapThreshold(int threshold);
        QRectF visibleRange() const;
        void setVisibleRange(const QRectF &range);
        qreal minimumDuration() const;
        void setMinimumDuration(qreal duration);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

        Q_INVOKABLE bool selectPhrase(sflow::NoteViewModel *firstNote, sflow::SelectionController *controller) const;

    Q_SIGNALS:
        void noteSequenceChanged();
        void gapThresholdChanged();
        void visibleRangeChanged();
        void minimumDurationChanged();

    private:
        enum Role {
            FirstNoteRole = Qt::UserRole + 1,
            StartTickRole,
            EndTickRole,
            LowestKeyRole,
            LyricsRole,
        };

        using NoteOrder = std::pair<int, quint64>;

        struct Note {
            sflow::NoteViewModel *viewModel;
            NoteOrder order;
            int length;
            int key;
            qint64 precedingGap = 0;
        };

        using OrderedNotes = std::map<NoteOrder, Note *>;
        using PhraseStarts = std::map<NoteOrder, OrderedNotes::const_iterator>;

        struct Phrase {
            NoteOrder start;
            QPointer<sflow::NoteViewModel> firstNote;
            qint64 endTick;
            int lowestKey;
            QString lyrics;

            bool operator==(const Phrase &) const = default;
        };

        void clearNotes();
        void addNote(QObject *item);
        void removeNote(QObject *item);
        void updateNote(QObject *item);
        void insertIndexedNote(Note &note);
        void removeIndexedNote(Note &note);
        void removeKey(const Note &note);
        void updateGap(OrderedNotes::const_iterator note);
        void setPhraseStart(const NoteOrder &order, bool startsPhrase);
        OrderedNotes::const_iterator phraseEnd(PhraseStarts::const_iterator start) const;
        void invalidateLyrics(const NoteOrder &order);
        int lowestKey(OrderedNotes::const_iterator first, OrderedNotes::const_iterator last) const;
        QString lyrics(OrderedNotes::const_iterator first, OrderedNotes::const_iterator end);
        void scheduleRefresh();
        void refresh();
        void updateRows(const QList<Phrase> &phrases);

        QPointer<sflow::RangeSequenceViewModel> m_noteSequence;
        int m_gapThreshold = 240;
        QRectF m_visibleRange;
        qreal m_minimumDuration = 0;
        quint64 m_nextNoteId = 0;
        bool m_refreshScheduled = false;
        std::unordered_map<sflow::NoteViewModel *, Note> m_noteRecords;
        OrderedNotes m_notes;
        std::map<int, std::set<NoteOrder>> m_notesByKey;
        std::set<std::pair<qint64, NoteOrder>> m_gapLengths;
        PhraseStarts m_phraseStarts;
        QHash<quint64, QString> m_phraseLyrics;
        QList<Phrase> m_rows;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_LYRICPHRASEMODEL_P_H
