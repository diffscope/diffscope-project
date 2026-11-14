// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ShortNoteProblemAddOn.h"

#include <algorithm>
#include <limits>

#include <QMetaObject>
#include <QVariant>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/BehaviorPreference.h>

#include <dspxproject/Document.h>
#include <dspxproject/DocumentProblemManager.h>
#include <dspxproject/ProjectDocumentContext.h>

#include <transactional/TransactionController.h>

namespace Core::Internal {

    static const auto shortNoteType = QStringLiteral("org.diffscope.core.shortNote");
    static const auto shortGapType = QStringLiteral("org.diffscope.core.shortGap");

    static qint64 gapLength(const dspx::Note *left, const dspx::Note *right) {
        return static_cast<qint64>(right->position()) - left->position() - left->length();
    }

    ShortNoteProblemAddOn::ShortNoteProblemAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ShortNoteProblemAddOn::~ShortNoteProblemAddOn() = default;

    void ShortNoteProblemAddOn::initialize() {
        m_document = windowHandle()->cast<ProjectWindowInterface>()->projectDocumentContext()->document();
        m_noteThreshold = BehaviorPreference::shortNoteThreshold();
        m_gapThreshold = BehaviorPreference::shortGapThreshold();

        auto *controller = m_document->transactionController();
        auto *preferences = BehaviorPreference::instance();
        connect(controller, &TransactionController::accessStateChanged, this, &ShortNoteProblemAddOn::scheduleRefresh);
        connect(preferences, &BehaviorPreference::shortNoteThresholdChanged, this, &ShortNoteProblemAddOn::scheduleRefresh);
        connect(preferences, &BehaviorPreference::shortGapThresholdChanged, this, &ShortNoteProblemAddOn::scheduleRefresh);

        auto *tracks = m_document->model()->tracks();
        connect(tracks, &dspx::TrackList::itemInserted, this, [this](int, dspx::Track *track) { watchTrack(track); });
        connect(tracks, &dspx::TrackList::itemRemoved, this, [this](int, dspx::Track *track) { unwatchTrack(track); });
        for (auto *track : tracks->items())
            watchTrack(track);

        auto *manager = m_document->problemManager();
        const auto canFill = [controller](const DspxProject::DocumentProblem &) {
            return controller->canBeginTransaction() && controller->canAcquireReadLock();
        };
        manager->registerAction(this, shortGapType, tr("Extend Previous Note Forward"),
                                [this](const auto &problem) { return fillGap(problem, true); }, canFill);
        manager->registerAction(this, shortGapType, tr("Extend Next Note Backward"),
                                [this](const auto &problem) { return fillGap(problem, false); }, canFill);
    }

    void ShortNoteProblemAddOn::extensionsInitialized() {
    }


    void ShortNoteProblemAddOn::watchTrack(dspx::Track *track) {
        const auto handle = track->handle();
        auto *clips = track->clips();
        const auto insert = [this, handle](dspx::Clip *clip) {
            if (qobject_cast<dspx::SingingClip *>(clip)) {
                m_trackClips[handle].insert(clip->handle());
                m_dirtyClips.insert(clip->handle());
                scheduleRefresh();
            }
        };
        connect(clips, &dspx::ClipSequence::itemInserted, this, insert);
        connect(clips, &dspx::ClipSequence::itemRemoved, this, [this, handle](dspx::Clip *clip) {
            if (m_trackClips[handle].remove(clip->handle())) {
                m_dirtyClips.insert(clip->handle());
                scheduleRefresh();
            }
        });
        for (auto *clip : clips->asRange())
            insert(clip);
    }

    void ShortNoteProblemAddOn::unwatchTrack(dspx::Track *track) {
        disconnect(track->clips(), nullptr, this, nullptr);
        m_dirtyClips.unite(m_trackClips.take(track->handle()));
        scheduleRefresh();
    }

    void ShortNoteProblemAddOn::markNoteDirty(dspx::Note *note) {
        m_dirtyNotes.insert(note->handle());
        scheduleRefresh();
    }

    void ShortNoteProblemAddOn::scheduleRefresh() {
        if (m_refreshQueued || !m_document || !m_document->transactionController()->canAcquireReadLock())
            return;
        if (m_dirtyClips.isEmpty() && m_dirtyNotes.isEmpty() &&
            m_noteThreshold == BehaviorPreference::shortNoteThreshold() && m_gapThreshold == BehaviorPreference::shortGapThreshold())
            return;
        m_refreshQueued = true;
        QMetaObject::invokeMethod(this, &ShortNoteProblemAddOn::refreshProblems, Qt::QueuedConnection);
    }

    void ShortNoteProblemAddOn::refreshClip(dspx::Handle handle) {
        auto *clip = m_document->model()->find<dspx::SingingClip>(handle);
        auto *sequence = clip ? clip->clipSequence() : nullptr;
        if (!sequence || sequence->track()->trackList() != m_document->model()->tracks())
            clip = nullptr;

        const auto old = m_clips.find(handle);
        if (old != m_clips.end()) {
            if (clip && old->clip == clip)
                return;
            if (old->clip)
                disconnect(old->clip->notes(), nullptr, this, nullptr);
            m_dirtyNotes.unite(old->notes);
            m_clips.erase(old);
        }
        if (!clip)
            return;

        auto *notes = clip->notes();
        m_clips.insert(handle, ClipState{clip, {}});
        connect(notes, &dspx::NoteSequence::itemInserted, this, &ShortNoteProblemAddOn::markNoteDirty);
        connect(notes, &dspx::NoteSequence::itemRemoved, this, &ShortNoteProblemAddOn::markNoteDirty);
        for (auto *note : notes->asRange())
            m_dirtyNotes.insert(note->handle());
    }

    void ShortNoteProblemAddOn::refreshNote(dspx::Handle handle) {
        auto *note = m_document->model()->find<dspx::Note>(handle);
        auto *sequence = note ? note->noteSequence() : nullptr;
        auto *clip = sequence ? sequence->singingClip() : nullptr;
        const auto currentClip = clip ? m_clips.find(clip->handle()) : m_clips.end();
        if (currentClip == m_clips.end() || currentClip->clip != clip)
            note = nullptr;

        auto entry = m_notes.find(handle);
        if (entry != m_notes.end()) {
            m_noteLengths.erase({entry->length, handle});
            if (entry->gap > 0)
                m_gapLengths.erase({entry->gap, handle});
            if (const auto oldClip = m_clips.find(entry->clip); oldClip != m_clips.end())
                oldClip->notes.remove(handle);
            if (entry->note && entry->note != note)
                disconnect(entry->note, nullptr, this, nullptr);
        }
        if (!note) {
            if (entry != m_notes.end())
                m_notes.erase(entry);
            return;
        }

        if (entry == m_notes.end() || entry->note != note) {
            const auto changed = [this, note] { markNoteDirty(note); };
            connect(note, &dspx::Note::positionChanged, this, changed);
            connect(note, &dspx::Note::lengthChanged, this, changed);
            connect(note, &dspx::Note::nextItemChanged, this, changed);
        }
        auto *next = note->nextItem();
        const auto length = note->length();
        const auto gap = next && next->noteSequence() == sequence ? gapLength(note, next) : 0;
        m_notes.insert(handle, NoteState{note, clip->handle(), length, gap});
        currentClip->notes.insert(handle);
        m_noteLengths.emplace(length, handle);
        if (gap > 0)
            m_gapLengths.emplace(gap, handle);
    }

    void ShortNoteProblemAddOn::refreshProblems() {
        m_refreshQueued = false;
        if (!m_document || !m_document->transactionController()->canAcquireReadLock())
            return;

        for (const auto handle : std::exchange(m_dirtyClips, {}))
            refreshClip(handle);

        const auto dirtyNotes = std::exchange(m_dirtyNotes, {});
        QSet<dspx::Handle> affected = dirtyNotes;
        auto *model = m_document->model();
        for (const auto handle : dirtyNotes) {
            if (auto *note = model->find<dspx::Note>(handle)) {
                if (auto *previous = note->previousItem())
                    affected.insert(previous->handle());
            }
        }
        for (const auto handle : std::as_const(affected))
            refreshNote(handle);

        const auto updateThreshold = [&affected](const LengthIndex &index, int &threshold, int newThreshold) {
            if (threshold == newThreshold)
                return;
            const auto begin = index.lower_bound({std::min(threshold, newThreshold) + 1, {}});
            const auto end = index.lower_bound({std::max(threshold, newThreshold) + 1, {}});
            for (auto it = begin; it != end; ++it)
                affected.insert(it->second);
            threshold = newThreshold;
        };
        updateThreshold(m_noteLengths, m_noteThreshold, BehaviorPreference::shortNoteThreshold());
        updateThreshold(m_gapLengths, m_gapThreshold, BehaviorPreference::shortGapThreshold());

        QList<DspxProject::DocumentProblemReport> reports;
        QStringList removed;
        for (const auto handle : std::as_const(affected)) {
            const auto prefix = QString::number(handle.d);
            const auto noteKey = prefix + QStringLiteral("/note");
            const auto leftKey = prefix + QStringLiteral("/gap-left");
            const auto rightKey = prefix + QStringLiteral("/gap-right");
            const auto state = m_notes.constFind(handle);
            auto *note = state == m_notes.cend() ? nullptr : state->note.data();
            if (note && state->length <= m_noteThreshold) {
                reports.append({noteKey, {shortNoteType, DspxProject::DocumentProblemSeverity::Notice,
                                           tr("There is a short note here."), note, {}}});
            } else {
                removed.append(noteKey);
            }
            if (note && state->gap > 0 && state->gap <= m_gapThreshold) {
                auto *next = note->nextItem();
                const QVariant data = QVariantList{QVariant::fromValue(handle.d), QVariant::fromValue(next->handle().d)};
                reports.append({leftKey, {shortGapType, DspxProject::DocumentProblemSeverity::Notice,
                                           tr("There is a short gap after this note."), note, data}});
                reports.append({rightKey, {shortGapType, DspxProject::DocumentProblemSeverity::Notice,
                                            tr("There is a short gap before this note."), next, data}});
            } else {
                removed.append(leftKey);
                removed.append(rightKey);
            }
        }
        if (!affected.isEmpty())
            m_document->problemManager()->updateProblems(this, reports, removed);
    }

    bool ShortNoteProblemAddOn::fillGap(const DspxProject::DocumentProblem &problem, bool extendPrevious) {
        const auto handles = problem.info.data.toList();
        if (handles.size() != 2)
            return false;
        auto *model = m_document->model();
        auto *left = model->find<dspx::Note>({handles.at(0).toULongLong()});
        auto *right = left ? left->nextItem() : nullptr;
        if (!right || !left->noteSequence() || right->noteSequence() != left->noteSequence() ||
            right->handle().d != handles.at(1).toULongLong())
            return false;
        const auto gap = gapLength(left, right);
        auto *note = extendPrevious ? left : right;
        const auto length = note->length() + gap;
        if (gap <= 0 || gap > BehaviorPreference::shortGapThreshold() || length > std::numeric_limits<int>::max())
            return false;

        return m_document->transactionController()->beginScopedTransaction(tr("Fill Gap"), [=] {
            if (!extendPrevious)
                note->setPosition(right->position() - static_cast<int>(gap));
            note->setLength(static_cast<int>(length));
            return true;
        });
    }

}

#include "moc_ShortNoteProblemAddOn.cpp"
