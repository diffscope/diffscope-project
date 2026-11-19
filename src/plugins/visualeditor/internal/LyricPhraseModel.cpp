// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LyricPhraseModel_p.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

#include <QMetaObject>
#include <QStringList>
#include <QVariant>

namespace VisualEditor::Internal {

    LyricPhraseModel::LyricPhraseModel(QObject *parent) : QAbstractListModel(parent) {
    }

    sflow::RangeSequenceViewModel *LyricPhraseModel::noteSequence() const {
        return m_noteSequence;
    }

    void LyricPhraseModel::setNoteSequence(sflow::RangeSequenceViewModel *noteSequence) {
        if (m_noteSequence == noteSequence)
            return;
        if (m_noteSequence)
            disconnect(m_noteSequence, nullptr, this, nullptr);
        clearNotes();
        m_noteSequence = noteSequence;
        if (m_noteSequence) {
            connect(noteSequence, &sflow::RangeSequenceViewModel::itemInserted, this, &LyricPhraseModel::addNote);
            connect(noteSequence, &sflow::RangeSequenceViewModel::itemRemoved, this, &LyricPhraseModel::removeNote);
            connect(noteSequence, &sflow::RangeSequenceViewModel::itemUpdated, this, &LyricPhraseModel::updateNote);
            connect(noteSequence, &QObject::destroyed, this, [this] {
                clearNotes();
                Q_EMIT noteSequenceChanged();
            });
            for (auto item : noteSequence->items())
                addNote(item);
        }
        scheduleRefresh();
        Q_EMIT noteSequenceChanged();
    }

    int LyricPhraseModel::gapThreshold() const {
        return m_gapThreshold;
    }

    void LyricPhraseModel::setGapThreshold(int threshold) {
        threshold = std::clamp(threshold, 1, 1920);
        if (m_gapThreshold == threshold)
            return;
        const auto [low, high] = std::minmax(m_gapThreshold, threshold);
        constexpr NoteOrder firstOrder{std::numeric_limits<int>::min(), 0};
        const auto end = m_gapLengths.lower_bound({high, firstOrder});
        for (auto gap = m_gapLengths.lower_bound({low, firstOrder}); gap != end; ++gap)
            setPhraseStart(gap->second, gap->first >= threshold);
        m_gapThreshold = threshold;
        scheduleRefresh();
        Q_EMIT gapThresholdChanged();
    }

    QRectF LyricPhraseModel::visibleRange() const {
        return m_visibleRange;
    }

    void LyricPhraseModel::setVisibleRange(const QRectF &range) {
        if (m_visibleRange == range)
            return;
        m_visibleRange = range;
        scheduleRefresh();
        Q_EMIT visibleRangeChanged();
    }

    qreal LyricPhraseModel::minimumDuration() const {
        return m_minimumDuration;
    }

    void LyricPhraseModel::setMinimumDuration(qreal duration) {
        if (m_minimumDuration == duration)
            return;
        m_minimumDuration = duration;
        scheduleRefresh();
        Q_EMIT minimumDurationChanged();
    }

    int LyricPhraseModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
    }

    QVariant LyricPhraseModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() >= m_rows.size())
            return {};
        const auto &phrase = m_rows.at(index.row());
        switch (role) {
            case FirstNoteRole:
                return QVariant::fromValue(phrase.firstNote.data());
            case StartTickRole:
                return phrase.start.first;
            case EndTickRole:
                return static_cast<qreal>(phrase.endTick);
            case LowestKeyRole:
                return phrase.lowestKey;
            case LyricsRole:
                return phrase.lyrics;
            default:
                return {};
        }
    }

    QHash<int, QByteArray> LyricPhraseModel::roleNames() const {
        return {{FirstNoteRole, "firstNote"}, {StartTickRole, "startTick"}, {EndTickRole, "endTick"}, {LowestKeyRole, "lowestKey"}, {LyricsRole, "lyrics"}};
    }

    bool LyricPhraseModel::selectPhrase(sflow::NoteViewModel *firstNote, sflow::SelectionController *controller) const {
        const auto note = m_noteRecords.find(firstNote);
        if (note == m_noteRecords.end() || !controller)
            return false;
        const auto start = m_phraseStarts.find(note->second.order);
        if (start == m_phraseStarts.end())
            return false;
        QList<QPointer<sflow::NoteViewModel>> notes;
        const auto end = phraseEnd(start);
        for (auto it = start->second; it != end; ++it)
            notes.append(QPointer(it->second->viewModel));

        using Controller = sflow::SelectionController;
        controller->select(firstNote, Controller::Select | Controller::ClearPreviousSelection | Controller::SetCurrentItem);
        for (qsizetype i = 1; i < notes.size(); ++i) {
            if (notes.at(i))
                controller->select(notes.at(i), Controller::Select);
        }
        return true;
    }

    void LyricPhraseModel::clearNotes() {
        for (const auto &entry : m_noteRecords)
            disconnect(entry.first, nullptr, this, nullptr);
        m_phraseStarts.clear();
        m_notes.clear();
        m_noteRecords.clear();
        m_notesByKey.clear();
        m_gapLengths.clear();
        m_phraseLyrics.clear();
        updateRows({});
    }

    void LyricPhraseModel::addNote(QObject *item) {
        auto viewModel = qobject_cast<sflow::NoteViewModel *>(item);
        if (!viewModel)
            return;
        auto [record, inserted] = m_noteRecords.try_emplace(viewModel, Note{viewModel, {viewModel->position(), ++m_nextNoteId}, viewModel->length(), viewModel->key()});
        if (!inserted)
            return;
        insertIndexedNote(record->second);
        connect(viewModel, &sflow::NoteViewModel::keyChanged, this, [this, viewModel] {
            auto &note = m_noteRecords.at(viewModel);
            removeKey(note);
            note.key = viewModel->key();
            m_notesByKey[note.key].insert(note.order);
            scheduleRefresh();
        });
        connect(viewModel, &sflow::NoteViewModel::lyricChanged, this, [this, viewModel] {
            invalidateLyrics(m_noteRecords.at(viewModel).order);
            scheduleRefresh();
        });
        scheduleRefresh();
    }

    void LyricPhraseModel::removeNote(QObject *item) {
        const auto record = m_noteRecords.find(qobject_cast<sflow::NoteViewModel *>(item));
        if (record == m_noteRecords.end())
            return;
        disconnect(item, nullptr, this, nullptr);
        removeIndexedNote(record->second);
        m_noteRecords.erase(record);
        scheduleRefresh();
    }

    void LyricPhraseModel::updateNote(QObject *item) {
        const auto record = m_noteRecords.find(qobject_cast<sflow::NoteViewModel *>(item));
        if (record == m_noteRecords.end())
            return;
        auto &note = record->second;
        const NoteOrder nextOrder{note.viewModel->position(), note.order.second};
        const int length = note.viewModel->length();
        if (nextOrder == note.order) {
            note.length = length;
            updateGap(std::next(m_notes.find(note.order)));
        } else {
            const auto current = m_notes.find(note.order);
            const auto destination = m_notes.lower_bound(nextOrder);
            const bool sameOrder = destination == current || destination == std::next(current);
            const auto start = std::prev(m_phraseStarts.upper_bound(note.order));
            const auto firstId = start->first.second;
            const auto lastId = std::prev(phraseEnd(start))->first.second;
            // The cached text remains valid if the phrase retains its notes in the same order.
            const auto cachedLyrics = sameOrder ? m_phraseLyrics.value(firstId) : QString{};
            removeIndexedNote(note);
            note.order = nextOrder;
            note.length = length;
            insertIndexedNote(note);
            const auto nextStart = std::prev(m_phraseStarts.upper_bound(note.order));
            if (!cachedLyrics.isNull() && nextStart->first.second == firstId && std::prev(phraseEnd(nextStart))->first.second == lastId)
                m_phraseLyrics.insert(firstId, cachedLyrics);
        }
        scheduleRefresh();
    }

    void LyricPhraseModel::insertIndexedNote(Note &note) {
        const auto it = m_notes.emplace(note.order, &note).first;
        m_notesByKey[note.key].insert(note.order);
        updateGap(it);
        updateGap(std::next(it));
        invalidateLyrics(note.order);
    }

    void LyricPhraseModel::removeIndexedNote(Note &note) {
        invalidateLyrics(note.order);
        const auto it = m_notes.find(note.order);
        const auto next = std::next(it);
        if (m_phraseStarts.erase(note.order) && next != m_notes.end())
            m_phraseStarts.try_emplace(next->first, next);
        if (note.precedingGap > 0)
            m_gapLengths.erase({note.precedingGap, note.order});
        note.precedingGap = 0;
        removeKey(note);
        m_notes.erase(it);
        updateGap(next);
    }

    void LyricPhraseModel::removeKey(const Note &note) {
        const auto key = m_notesByKey.find(note.key);
        key->second.erase(note.order);
        if (key->second.empty())
            m_notesByKey.erase(key);
    }

    void LyricPhraseModel::updateGap(OrderedNotes::const_iterator it) {
        if (it == m_notes.end())
            return;
        auto &note = *it->second;
        const bool isFirst = it == m_notes.begin();
        const auto previous = isFirst ? nullptr : std::prev(it)->second;
        const qint64 gap = previous ? qint64(note.order.first) - previous->order.first - previous->length : 0;
        if (note.precedingGap != gap) {
            if (note.precedingGap > 0)
                m_gapLengths.erase({note.precedingGap, note.order});
            if (gap > 0)
                m_gapLengths.emplace(gap, note.order);
            note.precedingGap = gap;
        }
        setPhraseStart(note.order, isFirst || gap >= m_gapThreshold);
    }

    void LyricPhraseModel::setPhraseStart(const NoteOrder &order, bool startsPhrase) {
        const auto it = m_phraseStarts.lower_bound(order);
        const bool wasStart = it != m_phraseStarts.end() && it->first == order;
        if (wasStart == startsPhrase)
            return;
        if (it != m_phraseStarts.begin())
            m_phraseLyrics.remove(std::prev(it)->first.second);
        if (startsPhrase) {
            m_phraseStarts.emplace_hint(it, order, m_notes.find(order));
        } else {
            m_phraseLyrics.remove(order.second);
            m_phraseStarts.erase(it);
        }
    }

    LyricPhraseModel::OrderedNotes::const_iterator LyricPhraseModel::phraseEnd(PhraseStarts::const_iterator start) const {
        const auto next = std::next(start);
        return next == m_phraseStarts.end() ? m_notes.end() : next->second;
    }

    void LyricPhraseModel::invalidateLyrics(const NoteOrder &order) {
        const auto start = m_phraseStarts.upper_bound(order);
        if (start != m_phraseStarts.begin())
            m_phraseLyrics.remove(std::prev(start)->first.second);
    }

    int LyricPhraseModel::lowestKey(OrderedNotes::const_iterator first, OrderedNotes::const_iterator last) const {
        const int firstKey = first->second->key;
        if (first != last) {
            for (auto it = m_notesByKey.begin(); it != m_notesByKey.end() && it->first < firstKey; ++it) {
                const auto candidate = it->second.lower_bound(first->first);
                if (candidate != it->second.end() && *candidate <= last->first)
                    return it->first;
            }
        }
        return firstKey;
    }

    QString LyricPhraseModel::lyrics(OrderedNotes::const_iterator first, OrderedNotes::const_iterator end) {
        const auto id = first->first.second;
        const auto cached = m_phraseLyrics.constFind(id);
        if (cached != m_phraseLyrics.cend())
            return *cached;
        QStringList lyrics;
        for (auto it = first; it != end; ++it)
            lyrics.append(it->second->viewModel->lyric());
        return m_phraseLyrics.insert(id, lyrics.join(u' ')).value();
    }

    void LyricPhraseModel::scheduleRefresh() {
        if (m_refreshScheduled)
            return;
        m_refreshScheduled = true;
        QMetaObject::invokeMethod(this, &LyricPhraseModel::refresh, Qt::QueuedConnection);
    }

    void LyricPhraseModel::refresh() {
        m_refreshScheduled = false;
        QList<Phrase> phrases;
        if (!m_phraseStarts.empty() && !m_visibleRange.isEmpty()) {
            const auto leftTick = std::clamp(std::floor(m_visibleRange.left()), qreal(std::numeric_limits<int>::min()), qreal(std::numeric_limits<int>::max()));
            auto start = m_phraseStarts.lower_bound({static_cast<int>(leftTick), 0});
            // Only the preceding phrase can intersect the left edge.
            if (start != m_phraseStarts.begin())
                --start;
            for (; start != m_phraseStarts.end() && start->first.first < m_visibleRange.right(); ++start) {
                const auto first = start->second;
                const auto end = phraseEnd(start);
                const auto last = std::prev(end);
                const auto endTick = qint64(last->first.first) + last->second->length;
                if (endTick <= m_visibleRange.left() || endTick - start->first.first <= m_minimumDuration)
                    continue;
                const int key = lowestKey(first, last);
                if (key >= m_visibleRange.top() && key <= m_visibleRange.bottom())
                    phrases.append({start->first, first->second->viewModel, endTick, key, lyrics(first, end)});
            }
        }
        updateRows(phrases);
    }

    void LyricPhraseModel::updateRows(const QList<Phrase> &phrases) {
        int row = 0;
        while (row < m_rows.size() || row < phrases.size()) {
            if (row < m_rows.size() && row < phrases.size() && m_rows.at(row).start == phrases.at(row).start) {
                if (m_rows.at(row) != phrases.at(row)) {
                    m_rows[row] = phrases.at(row);
                    Q_EMIT dataChanged(index(row), index(row));
                }
                ++row;
            } else if (row == phrases.size() || (row < m_rows.size() && m_rows.at(row).start < phrases.at(row).start)) {
                int end = row + 1;
                while (end < m_rows.size() && (row == phrases.size() || m_rows.at(end).start < phrases.at(row).start))
                    ++end;
                beginRemoveRows({}, row, end - 1);
                m_rows.remove(row, end - row);
                endRemoveRows();
            } else {
                int end = row + 1;
                while (end < phrases.size() && (row == m_rows.size() || phrases.at(end).start < m_rows.at(row).start))
                    ++end;
                beginInsertRows({}, row, end - 1);
                m_rows.insert(row, end - row, Phrase{});
                std::copy(phrases.cbegin() + row, phrases.cbegin() + end, m_rows.begin() + row);
                endInsertRows();
                row = end;
            }
        }
    }

}

#include "moc_LyricPhraseModel_p.cpp"
