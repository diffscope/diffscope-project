#include "PhonemeViewModelContextData_p.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#include <QLoggingCategory>
#include <QMetaObject>
#include <QQuickItem>

#include <ScopicFlowCore/NoteViewModel.h>
#include <ScopicFlowCore/PhonemeSequenceInteractionController.h>
#include <ScopicFlowCore/PhonemeViewModel.h>
#include <ScopicFlowCore/PointSequenceViewModel.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <opendspx/note.h>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    namespace {
        constexpr double InitialTempo = 120.0;

        int boundedInteger(qint64 value) {
            return static_cast<int>(qBound(std::numeric_limits<int>::min(), value, std::numeric_limits<int>::max()));
        }
    }

    Q_STATIC_LOGGING_CATEGORY(lcPhonemeViewModelContextData, "diffscope.visualeditor.phonemeviewmodelcontextdata")

    PhonemeViewModelContextData::NoteInterval::NoteInterval()
        : lib_interval_tree::interval<int>(0, 0) {
    }

    PhonemeViewModelContextData::NoteInterval::NoteInterval(int start, int end, dspx::Note *note)
        : lib_interval_tree::interval<int>(start, qMax(start, end)), m_note(note) {
    }

    dspx::Note *PhonemeViewModelContextData::NoteInterval::note() const {
        return m_note;
    }

    bool PhonemeViewModelContextData::NoteInterval::operator==(const NoteInterval &other) const {
        return m_note == other.m_note;
    }

    PhonemeViewModelContextData::~PhonemeViewModelContextData() {
        if (document &&
            transactionId != Core::TransactionController::TransactionId::Invalid) {
            document->transactionController()->abortTransaction(transactionId);
        }
        for (auto *binding : noteBindings) {
            delete binding;
        }
    }

    void PhonemeViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        document = q->windowHandle()->projectDocumentContext()->document();
        musicTimeline = q->windowHandle()->projectTimeline()->musicTimeline();
        tempoSequence = document->model()->tempos();
        trackList = document->model()->tracks();

        connect(tempoSequence, &dspx::TempoSequence::itemInserted, this,
                [this](dspx::Tempo *tempo) {
                    bindTempo(tempo);
                    addDirtyTempoRange(tempo->position(), tempoSegmentEnd(tempo->position()));
                });
        connect(tempoSequence, &dspx::TempoSequence::itemRemoved, this,
                [this](dspx::Tempo *tempo) {
                    unbindTempo(tempo);
                });
        for (auto *tempo : tempoSequence->asRange()) {
            bindTempo(tempo);
        }
    }

    void PhonemeViewModelContextData::bindTrackSequences() {
        connect(trackList, &dspx::TrackList::itemInserted, this,
                [this](int, dspx::Track *track) {
                    bindTrack(track);
                });
        connect(trackList, &dspx::TrackList::itemRemoved, this,
                [this](int, dspx::Track *track) {
                    unbindTrack(track);
                });
        for (auto *track : trackList->items()) {
            bindTrack(track);
        }
    }

    void PhonemeViewModelContextData::bindTrack(dspx::Track *track) {
        if (!track || boundTracks.contains(track)) {
            return;
        }
        boundTracks.insert(track);
        auto *sequence = track->clips();
        connect(sequence, &dspx::ClipSequence::itemInserted, this,
                [this](dspx::Clip *clip) {
                    if (auto *singingClip = qobject_cast<dspx::SingingClip *>(clip)) {
                        bindSingingClip(singingClip);
                    }
                });
        connect(sequence, &dspx::ClipSequence::itemRemoved, this,
                [this](dspx::Clip *clip, dspx::ClipSequence *sequenceToWhichMoved) {
                    auto *singingClip = qobject_cast<dspx::SingingClip *>(clip);
                    if (!singingClip) {
                        return;
                    }
                    if (sequenceToWhichMoved &&
                        boundTracks.contains(sequenceToWhichMoved->track())) {
                        return;
                    }
                    unbindSingingClip(singingClip);
                });
        for (auto *clip : sequence->asRange()) {
            if (auto *singingClip = qobject_cast<dspx::SingingClip *>(clip)) {
                bindSingingClip(singingClip);
            }
        }
    }

    void PhonemeViewModelContextData::unbindTrack(dspx::Track *track) {
        if (!track || !boundTracks.remove(track)) {
            return;
        }
        auto *sequence = track->clips();
        disconnect(sequence, nullptr, this, nullptr);
        const auto clips = phonemeSequenceViewModelMap.keys();
        for (auto *clip : clips) {
            if (clip->clipSequence() && clip->clipSequence()->track() == track) {
                unbindSingingClip(clip);
            }
        }
    }

    void PhonemeViewModelContextData::bindSingingClip(dspx::SingingClip *clip) {
        if (!clip || phonemeSequenceViewModelMap.contains(clip)) {
            return;
        }
        auto *sequenceViewModel = new sflow::PointSequenceViewModel(q_ptr, "sequencePosition");
        phonemeSequenceViewModelMap.insert(clip, sequenceViewModel);
        clipNotes.insert(clip, {});

        auto *noteSequence = clip->notes();
        connect(noteSequence, &dspx::NoteSequence::itemInserted, this,
                [this](dspx::Note *note) {
                    bindNote(note);
                });
        connect(noteSequence, &dspx::NoteSequence::itemRemoved, this,
                [this](dspx::Note *note) {
                    unbindNote(note);
                });
        connect(clip, &dspx::Clip::positionChanged, this, [this, clip] {
            scheduleClipUpdate(clip);
        });
        connect(clip, &dspx::Clip::clipStartChanged, this, [this, clip] {
            scheduleClipUpdate(clip);
        });

        for (auto *note : noteSequence->asRange()) {
            bindNote(note);
        }
    }

    void PhonemeViewModelContextData::unbindSingingClip(dspx::SingingClip *clip) {
        if (!clip || !phonemeSequenceViewModelMap.contains(clip)) {
            return;
        }
        disconnect(clip->notes(), nullptr, this, nullptr);
        disconnect(clip, nullptr, this, nullptr);
        const auto notes = clipNotes.value(clip);
        for (auto *note : notes) {
            unbindNote(note);
        }
        clipNotes.remove(clip);
        dirtyClips.remove(clip);
        auto *sequenceViewModel = phonemeSequenceViewModelMap.take(clip);
        if (sequenceViewModel) {
            sequenceViewModel->deleteLater();
        }
    }

    void PhonemeViewModelContextData::bindNote(dspx::Note *note) {
        if (!note || noteBindings.contains(note) || !note->noteSequence()) {
            return;
        }
        auto *clip = note->noteSequence()->singingClip();
        if (!phonemeSequenceViewModelMap.contains(clip)) {
            return;
        }
        auto *binding = new NoteBinding;
        binding->note = note;
        binding->clip = clip;
        noteBindings.insert(note, binding);
        clipNotes[clip].insert(note);

        connect(note, &dspx::Note::positionChanged, this, [this, note] {
            scheduleNoteUpdate(note);
        });
        const auto rebuild = [this, note] {
            if (!committingDocumentChanges) {
                scheduleNoteRebuild(note);
            }
        };
        connect(note->originalPhonemes(), &dspx::PhonemeSequence::itemInserted, this,
                [rebuild](dspx::Phoneme *, dspx::PhonemeSequence *) {
                    rebuild();
                });
        connect(note->originalPhonemes(), &dspx::PhonemeSequence::itemRemoved, this,
                [rebuild](dspx::Phoneme *, dspx::PhonemeSequence *) {
                    rebuild();
                });
        connect(note->editedPhonemes(), &dspx::PhonemeSequence::itemInserted, this,
                [rebuild](dspx::Phoneme *, dspx::PhonemeSequence *) {
                    rebuild();
                });
        connect(note->editedPhonemes(), &dspx::PhonemeSequence::itemRemoved, this,
                [rebuild](dspx::Phoneme *, dspx::PhonemeSequence *) {
                    rebuild();
                });

        rebuildNote(note);
    }

    void PhonemeViewModelContextData::unbindNote(dspx::Note *note) {
        auto *binding = noteBindings.value(note);
        if (!binding) {
            return;
        }
        if (targetViewItem &&
            binding->viewItems.contains(targetViewItem.data())) {
            abortEdit();
        }
        dirtyNoteRebuilds.remove(note);
        dirtyNotes.remove(note);
        clipNotes[binding->clip].remove(note);
        disconnect(note, nullptr, this, nullptr);
        disconnect(note->originalPhonemes(), nullptr, this, nullptr);
        disconnect(note->editedPhonemes(), nullptr, this, nullptr);
        clearActivePhonemes(binding);
        noteBindings.remove(note);
        delete binding;
    }

    void PhonemeViewModelContextData::rebuildNote(dspx::Note *note) {
        auto *binding = noteBindings.value(note);
        if (!binding) {
            return;
        }
        if (targetViewItem &&
            binding->viewItems.contains(targetViewItem.data())) {
            abortEdit();
        }
        clearActivePhonemes(binding);
        binding->edited = note->editedPhonemes()->size() > 0;
        binding->activeSequence = binding->edited
            ? note->editedPhonemes()
            : note->originalPhonemes();
        for (auto *phoneme : binding->activeSequence->asRange()) {
            bindActivePhoneme(binding, phoneme);
        }
        rebuildLiveOrder(binding);
        updateNoteInterval(binding);
    }

    void PhonemeViewModelContextData::clearActivePhonemes(NoteBinding *binding) {
        if (!binding) {
            return;
        }
        removeNoteInterval(binding->note);
        auto *sequenceViewModel = phonemeSequenceViewModelMap.value(binding->clip);
        const auto viewItems = binding->viewItems;
        for (auto *viewItem : viewItems) {
            auto *phoneme = phonemeDocumentItemMap.take(viewItem);
            if (phoneme) {
                phonemeViewItemMap.remove(phoneme);
                disconnect(phoneme, nullptr, this, nullptr);
            }
            if (sequenceViewModel) {
                sequenceViewModel->removeItem(viewItem);
            }
            viewItem->setNextPhoneme(nullptr);
            viewItem->deleteLater();
        }
        binding->viewItems.clear();
        binding->stableOrders.clear();
        binding->orderKeys.clear();
        binding->order.clear();
        binding->nextStableOrder = 0;
    }

    void PhonemeViewModelContextData::bindActivePhoneme(NoteBinding *binding,
                                                        dspx::Phoneme *phoneme) {
        Q_Q(ProjectViewModelContext);
        auto *sequenceViewModel = phonemeSequenceViewModelMap.value(binding->clip);
        if (!sequenceViewModel || !phoneme) {
            return;
        }
        auto *viewItem = new sflow::PhonemeViewModel(sequenceViewModel);
        phonemeViewItemMap.insert(phoneme, viewItem);
        phonemeDocumentItemMap.insert(viewItem, phoneme);
        binding->viewItems.append(viewItem);
        binding->stableOrders.insert(viewItem, binding->nextStableOrder++);

        ++syncingViewDepth;
        viewItem->setAssociatedNote(q->getNoteViewItemFromDocumentItem(binding->note));
        viewItem->setContent(phoneme->token());
        viewItem->setEdited(binding->edited);
        viewItem->setPosition(documentPositionToViewPosition(binding->note, phoneme->start()));
        --syncingViewDepth;

        connectActivePhoneme(phoneme, viewItem);
        insertIntoLiveOrder(binding, viewItem);
        sequenceViewModel->insertItem(viewItem);
    }

    void PhonemeViewModelContextData::connectActivePhoneme(
        dspx::Phoneme *phoneme, sflow::PhonemeViewModel *viewItem,
        bool connectViewItem) {
        connect(phoneme, &dspx::Phoneme::startChanged, this,
                [this, phoneme, viewItem] {
                    if (phonemeDocumentItemMap.value(viewItem) != phoneme) {
                        return;
                    }
                    auto *binding = bindingForViewItem(viewItem);
                    if (!binding) {
                        return;
                    }
                    ++syncingViewDepth;
                    viewItem->setPosition(
                        documentPositionToViewPosition(binding->note, phoneme->start()));
                    --syncingViewDepth;
                    updateLiveOrder(binding, viewItem);
                    updateNoteInterval(binding);
                });
        connect(phoneme, &dspx::Phoneme::tokenChanged, this,
                [this, phoneme, viewItem] {
                    if (phonemeDocumentItemMap.value(viewItem) != phoneme) {
                        return;
                    }
                    ++syncingViewDepth;
                    viewItem->setContent(phoneme->token());
                    --syncingViewDepth;
                });
        if (!connectViewItem) {
            return;
        }
        connect(viewItem, &sflow::PhonemeViewModel::positionChanged, this,
                [this, viewItem] {
                    handleViewPositionChanged(viewItem);
                });
        connect(viewItem, &sflow::PhonemeViewModel::contentChanged, this,
                [this, viewItem] {
                    handleViewContentChanged(viewItem);
                });
        connect(viewItem, &sflow::PhonemeViewModel::editedChanged, this,
                [this, viewItem] {
                    auto *binding = bindingForViewItem(viewItem);
                    if (!binding || viewItem->isEdited() == binding->edited) {
                        return;
                    }
                    ++syncingViewDepth;
                    viewItem->setEdited(binding->edited);
                    --syncingViewDepth;
                });
    }

    void PhonemeViewModelContextData::insertIntoLiveOrder(
        NoteBinding *binding, sflow::PhonemeViewModel *viewItem) {
        if (!binding || !viewItem || binding->orderKeys.contains(viewItem)) {
            return;
        }
        const OrderKey key {viewItem->position(), binding->stableOrders.value(viewItem)};
        auto next = binding->order.lower_bound(key);
        sflow::PhonemeViewModel *previousItem = nullptr;
        if (next != binding->order.begin()) {
            previousItem = std::prev(next)->second;
        }
        auto inserted = binding->order.emplace_hint(next, key, viewItem);
        binding->orderKeys.insert(viewItem, key);
        viewItem->setNextPhoneme(std::next(inserted) == binding->order.end()
                                    ? nullptr
                                    : std::next(inserted)->second);
        if (previousItem) {
            previousItem->setNextPhoneme(viewItem);
        }
    }

    void PhonemeViewModelContextData::removeFromLiveOrder(
        NoteBinding *binding, sflow::PhonemeViewModel *viewItem) {
        if (!binding || !viewItem || !binding->orderKeys.contains(viewItem)) {
            return;
        }
        const auto key = binding->orderKeys.take(viewItem);
        auto item = binding->order.find(key);
        if (item == binding->order.end()) {
            return;
        }
        sflow::PhonemeViewModel *previousItem = nullptr;
        if (item != binding->order.begin()) {
            previousItem = std::prev(item)->second;
        }
        const auto next = std::next(item);
        auto *nextItem = next == binding->order.end() ? nullptr : next->second;
        binding->order.erase(item);
        if (previousItem) {
            previousItem->setNextPhoneme(nextItem);
        }
        viewItem->setNextPhoneme(nullptr);
    }

    void PhonemeViewModelContextData::updateLiveOrder(
        NoteBinding *binding, sflow::PhonemeViewModel *viewItem) {
        removeFromLiveOrder(binding, viewItem);
        insertIntoLiveOrder(binding, viewItem);
    }

    void PhonemeViewModelContextData::rebuildLiveOrder(NoteBinding *binding) {
        if (!binding) {
            return;
        }
        binding->order.clear();
        binding->orderKeys.clear();
        for (auto *viewItem : binding->viewItems) {
            viewItem->setNextPhoneme(nullptr);
        }
        for (auto *viewItem : binding->viewItems) {
            insertIntoLiveOrder(binding, viewItem);
        }
    }

    void PhonemeViewModelContextData::bindTempo(dspx::Tempo *tempo) {
        if (!tempo || tempoPositions.contains(tempo)) {
            return;
        }
        tempoPositions.insert(tempo, tempo->position());
        tempoItems.insert_or_assign(tempo->position(), tempo);
        connect(tempo, &dspx::Tempo::positionChanged, this, [this, tempo] {
            handleTempoPositionChanged(tempo);
        });
        connect(tempo, &dspx::Tempo::valueChanged, this, [this, tempo] {
            const int position = tempoPositions.value(tempo, tempo->position());
            addDirtyTempoRange(position, tempoSegmentEnd(position));
        });
    }

    void PhonemeViewModelContextData::unbindTempo(dspx::Tempo *tempo) {
        if (!tempo || !tempoPositions.contains(tempo)) {
            return;
        }
        const int position = tempoPositions.take(tempo);
        const int oldEnd = tempoSegmentEnd(position);
        auto item = tempoItems.find(position);
        if (item != tempoItems.end() && item->second == tempo) {
            tempoItems.erase(item);
        }
        disconnect(tempo, nullptr, this, nullptr);
        addDirtyTempoRange(position, oldEnd);
    }

    void PhonemeViewModelContextData::handleTempoPositionChanged(dspx::Tempo *tempo) {
        if (!tempoPositions.contains(tempo)) {
            bindTempo(tempo);
            return;
        }
        const int oldPosition = tempoPositions.value(tempo);
        const int oldEnd = tempoSegmentEnd(oldPosition);
        auto oldItem = tempoItems.find(oldPosition);
        if (oldItem != tempoItems.end() && oldItem->second == tempo) {
            tempoItems.erase(oldItem);
        }

        const int newPosition = tempo->position();
        tempoItems.insert_or_assign(newPosition, tempo);
        tempoPositions.insert(tempo, newPosition);
        const int newEnd = tempoSegmentEnd(newPosition);
        addDirtyTempoRange(oldPosition, oldEnd);
        addDirtyTempoRange(newPosition, newEnd);
    }

    void PhonemeViewModelContextData::addDirtyTempoRange(int start, int end) {
        if (end <= start) {
            return;
        }
        dirtyTempoRanges.append({start, end});
        schedulePendingUpdate();
    }

    int PhonemeViewModelContextData::tempoSegmentEnd(int position) const {
        const auto item = tempoItems.upper_bound(position);
        return item == tempoItems.end() ? std::numeric_limits<int>::max() : item->first;
    }

    void PhonemeViewModelContextData::scheduleNoteRebuild(dspx::Note *note) {
        if (!noteBindings.contains(note)) {
            return;
        }
        dirtyNoteRebuilds.insert(note);
        schedulePendingUpdate();
    }

    void PhonemeViewModelContextData::scheduleNoteUpdate(dspx::Note *note) {
        if (!noteBindings.contains(note)) {
            return;
        }
        dirtyNotes.insert(note);
        schedulePendingUpdate();
    }

    void PhonemeViewModelContextData::scheduleClipUpdate(dspx::SingingClip *clip) {
        if (!phonemeSequenceViewModelMap.contains(clip)) {
            return;
        }
        dirtyClips.insert(clip);
        schedulePendingUpdate();
    }

    void PhonemeViewModelContextData::schedulePendingUpdate() {
        if (updateScheduled) {
            return;
        }
        updateScheduled = true;
        QMetaObject::invokeMethod(this, [this] {
            flushPendingUpdates();
        }, Qt::QueuedConnection);
    }

    void PhonemeViewModelContextData::flushPendingUpdates() {
        updateScheduled = false;
        const auto notesToRebuild = std::exchange(dirtyNoteRebuilds, {});
        QSet<dspx::Note *> notes = std::exchange(dirtyNotes, {});
        const auto clips = std::exchange(dirtyClips, {});
        for (auto *clip : clips) {
            notes.unite(clipNotes.value(clip));
        }

        auto tempoRanges = std::exchange(dirtyTempoRanges, {});
        std::sort(tempoRanges.begin(), tempoRanges.end(), [](const auto &left, const auto &right) {
            return left.first < right.first ||
                   (left.first == right.first && left.second < right.second);
        });
        QList<QPair<int, int>> mergedRanges;
        for (const auto &range : tempoRanges) {
            if (mergedRanges.isEmpty() || range.first > mergedRanges.last().second) {
                mergedRanges.append(range);
            } else {
                mergedRanges.last().second = qMax(mergedRanges.last().second, range.second);
            }
        }
        for (const auto &range : mergedRanges) {
            const int inclusiveEnd = range.second == std::numeric_limits<int>::max()
                ? range.second
                : range.second - 1;
            NoteInterval query(range.first, inclusiveEnd, nullptr);
            noteIntervalTree.overlap_find_all(query, [&notes](const auto &item) {
                if (auto *note = item.interval().note()) {
                    notes.insert(note);
                }
                return true;
            });
        }

        for (auto *note : notesToRebuild) {
            if (noteBindings.contains(note)) {
                rebuildNote(note);
                notes.remove(note);
            }
        }
        for (auto *note : notes) {
            if (auto *binding = noteBindings.value(note)) {
                syncNotePositions(binding);
            }
        }
    }

    void PhonemeViewModelContextData::updateNoteInterval(NoteBinding *binding) {
        if (!binding) {
            return;
        }
        removeNoteInterval(binding->note);
        if (binding->viewItems.isEmpty()) {
            return;
        }
        const double notePosition = noteAbsoluteTick(binding->note);
        double minimum = notePosition;
        double maximum = notePosition;
        for (auto *viewItem : binding->viewItems) {
            const double position = notePosition + viewItem->position();
            minimum = qMin(minimum, position);
            maximum = qMax(maximum, position);
        }
        const int start = boundedInteger(static_cast<qint64>(std::floor(minimum)));
        const int end = boundedInteger(static_cast<qint64>(std::ceil(maximum)));
        NoteInterval interval(start, end, binding->note);
        noteIntervals.insert(binding->note, interval);
        noteIntervalTree.insert(interval);
    }

    void PhonemeViewModelContextData::removeNoteInterval(dspx::Note *note) {
        if (!noteIntervals.contains(note)) {
            return;
        }
        const auto interval = noteIntervals.take(note);
        noteIntervalTree.erase(noteIntervalTree.find(interval));
    }

    void PhonemeViewModelContextData::syncNotePositions(NoteBinding *binding) {
        if (!binding ||
            (targetViewItem &&
             binding->viewItems.contains(targetViewItem.data()))) {
            return;
        }
        ++syncingViewDepth;
        for (auto *viewItem : binding->viewItems) {
            if (auto *phoneme = phonemeDocumentItemMap.value(viewItem)) {
                viewItem->setPosition(
                    documentPositionToViewPosition(binding->note, phoneme->start()));
            }
        }
        --syncingViewDepth;
        rebuildLiveOrder(binding);
        updateNoteInterval(binding);
    }

    int PhonemeViewModelContextData::noteAbsoluteTick(const dspx::Note *note) const {
        if (!note || !note->noteSequence()) {
            return 0;
        }
        return boundedInteger(
            static_cast<qint64>(note->noteSequence()->singingClip()->start()) +
            note->position());
    }

    double PhonemeViewModelContextData::tickToMilliseconds(double tick) const {
        const double millisecondsPerTickAtInitialTempo =
            60.0 * 1000.0 / (musicTimeline->ticksPerQuarterNote() * InitialTempo);
        if (tick < 0.0) {
            return tick * millisecondsPerTickAtInitialTempo;
        }
        const double wholePart = std::floor(tick);
        const int wholeTick = boundedInteger(static_cast<qint64>(wholePart));
        const double fraction = tick - wholePart;
        const double start = musicTimeline->create(0, 0, wholeTick).millisecond();
        if (fraction == 0.0 || wholeTick == std::numeric_limits<int>::max()) {
            return start;
        }
        const double end = musicTimeline->create(0, 0, wholeTick + 1).millisecond();
        return start + fraction * (end - start);
    }

    double PhonemeViewModelContextData::millisecondsToTick(double milliseconds) const {
        if (milliseconds < 0.0) {
            const double ticksPerMillisecond =
                musicTimeline->ticksPerQuarterNote() * InitialTempo / (60.0 * 1000.0);
            return milliseconds * ticksPerMillisecond;
        }
        const int nearestTick = musicTimeline->create(milliseconds).totalTick();
        const double nearestMilliseconds = tickToMilliseconds(nearestTick);
        if (qFuzzyCompare(milliseconds + 1.0, nearestMilliseconds + 1.0)) {
            return nearestTick;
        }
        if (milliseconds > nearestMilliseconds &&
            nearestTick < std::numeric_limits<int>::max()) {
            const double nextMilliseconds = tickToMilliseconds(nearestTick + 1);
            return nearestTick +
                (milliseconds - nearestMilliseconds) /
                    (nextMilliseconds - nearestMilliseconds);
        }
        if (nearestTick > 0) {
            const double previousMilliseconds = tickToMilliseconds(nearestTick - 1);
            return nearestTick -
                (nearestMilliseconds - milliseconds) /
                    (nearestMilliseconds - previousMilliseconds);
        }
        return 0.0;
    }

    double PhonemeViewModelContextData::documentPositionToViewPosition(
        const dspx::Note *note, int milliseconds) const {
        const int notePosition = noteAbsoluteTick(note);
        const double noteMilliseconds = tickToMilliseconds(notePosition);
        return millisecondsToTick(noteMilliseconds + milliseconds) - notePosition;
    }

    int PhonemeViewModelContextData::viewPositionToDocumentPosition(
        const dspx::Note *note, double position) const {
        const int notePosition = noteAbsoluteTick(note);
        const double noteMilliseconds = tickToMilliseconds(notePosition);
        const double phonemeMilliseconds = tickToMilliseconds(notePosition + position);
        return boundedInteger(qRound64(phonemeMilliseconds - noteMilliseconds));
    }

    PhonemeViewModelContextData::NoteBinding *
    PhonemeViewModelContextData::bindingForViewItem(
        sflow::PhonemeViewModel *viewItem) const {
        auto *phoneme = phonemeDocumentItemMap.value(viewItem);
        return phoneme && phoneme->phonemeSequence()
            ? noteBindings.value(phoneme->phonemeSequence()->note())
            : nullptr;
    }

    void PhonemeViewModelContextData::handleViewPositionChanged(
        sflow::PhonemeViewModel *viewItem) {
        if (syncingViewDepth > 0) {
            return;
        }
        auto *binding = bindingForViewItem(viewItem);
        if (!binding) {
            return;
        }
        updateLiveOrder(binding, viewItem);
        updateNoteInterval(binding);
        if (editOperation != MoveEdit || targetViewItem != viewItem) {
            auto *phoneme = phonemeDocumentItemMap.value(viewItem);
            ++syncingViewDepth;
            viewItem->setPosition(
                documentPositionToViewPosition(binding->note, phoneme->start()));
            --syncingViewDepth;
            updateLiveOrder(binding, viewItem);
            updateNoteInterval(binding);
        }
    }

    void PhonemeViewModelContextData::handleViewContentChanged(
        sflow::PhonemeViewModel *viewItem) {
        if (syncingViewDepth > 0) {
            return;
        }
        if (editOperation != TokenEdit || targetViewItem != viewItem) {
            if (auto *phoneme = phonemeDocumentItemMap.value(viewItem)) {
                ++syncingViewDepth;
                viewItem->setContent(phoneme->token());
                --syncingViewDepth;
            }
        }
    }

    bool PhonemeViewModelContextData::beginEdit(
        EditOperation operation, QQuickItem *sequenceItem,
        sflow::PhonemeViewModel *viewItem) {
        if (editOperation != NoEdit || !sequenceItem || !viewItem ||
            !bindingForViewItem(viewItem)) {
            return false;
        }
        const auto id = document->transactionController()->beginTransaction();
        if (id == Core::TransactionController::TransactionId::Invalid) {
            return false;
        }
        editOperation = operation;
        targetViewItem = viewItem;
        targetSequenceItem = sequenceItem;
        targetSequenceDestroyedConnection =
            connect(sequenceItem, &QObject::destroyed, this, [this] {
                abortEdit();
            });
        originalViewPosition = viewItem->position();
        originalViewContent = viewItem->content();
        transactionId = id;
        return true;
    }

    void PhonemeViewModelContextData::commitMove() {
        auto *viewItem = targetViewItem.data();
        auto *binding = bindingForViewItem(viewItem);
        auto *phoneme = phonemeDocumentItemMap.value(viewItem);
        if (editOperation != MoveEdit || !binding || !phoneme) {
            abortEdit();
            return;
        }
        const int newPosition =
            viewPositionToDocumentPosition(binding->note, viewItem->position());
        if (newPosition == phoneme->start()) {
            document->transactionController()->abortTransaction(transactionId);
            restoreTargetViewItem();
            finishEdit();
            return;
        }

        committingDocumentChanges = true;
        phoneme = ensureEditedPhonemes(binding, viewItem);
        if (!phoneme) {
            committingDocumentChanges = false;
            document->transactionController()->abortTransaction(transactionId);
            restoreTargetViewItem();
            finishEdit();
            return;
        }
        phoneme->setStart(newPosition);
        document->transactionController()->commitTransaction(transactionId, tr("Moving phoneme"));
        committingDocumentChanges = false;
        updateNoteInterval(binding);
        finishEdit();
    }

    void PhonemeViewModelContextData::commitTokenEdit() {
        auto *viewItem = targetViewItem.data();
        auto *binding = bindingForViewItem(viewItem);
        auto *phoneme = phonemeDocumentItemMap.value(viewItem);
        if (editOperation != TokenEdit || !binding || !phoneme) {
            abortEdit();
            return;
        }
        if (viewItem->content() == phoneme->token()) {
            document->transactionController()->abortTransaction(transactionId);
            restoreTargetViewItem();
            finishEdit();
            return;
        }

        committingDocumentChanges = true;
        phoneme = ensureEditedPhonemes(binding, viewItem);
        if (!phoneme) {
            committingDocumentChanges = false;
            document->transactionController()->abortTransaction(transactionId);
            restoreTargetViewItem();
            finishEdit();
            return;
        }
        phoneme->setToken(viewItem->content());
        document->transactionController()->commitTransaction(transactionId, tr("Editing phoneme"));
        committingDocumentChanges = false;
        finishEdit();
    }

    void PhonemeViewModelContextData::abortEdit() {
        if (editOperation == NoEdit) {
            return;
        }
        document->transactionController()->abortTransaction(transactionId);
        restoreTargetViewItem();
        finishEdit();
    }

    void PhonemeViewModelContextData::finishEdit() {
        auto *binding = bindingForViewItem(targetViewItem);
        auto *note = binding ? binding->note : nullptr;
        disconnect(targetSequenceDestroyedConnection);
        targetSequenceDestroyedConnection = {};
        editOperation = NoEdit;
        targetViewItem = nullptr;
        targetSequenceItem = nullptr;
        originalViewPosition = 0.0;
        originalViewContent.clear();
        transactionId = Core::TransactionController::TransactionId::Invalid;
        scheduleNoteUpdate(note);
    }

    dspx::Phoneme *PhonemeViewModelContextData::ensureEditedPhonemes(
        NoteBinding *binding, sflow::PhonemeViewModel *target) {
        if (binding->edited) {
            return phonemeDocumentItemMap.value(target);
        }

        struct Replacement {
            dspx::Phoneme *original{};
            dspx::Phoneme *edited{};
            sflow::PhonemeViewModel *viewItem{};
        };
        QList<Replacement> replacements;
        QList<dspx::Phoneme *> originalPhonemes;
        for (auto *original : binding->note->originalPhonemes()->asRange()) {
            originalPhonemes.append(original);
        }
        auto *editedSequence = binding->note->editedPhonemes();
        editedSequence->fromOpenDSPX(
            binding->note->originalPhonemes()->toOpenDSPX());
        QList<dspx::Phoneme *> editedPhonemes;
        for (auto *edited : editedSequence->asRange()) {
            editedPhonemes.append(edited);
        }
        if (originalPhonemes.size() != editedPhonemes.size()) {
            qCWarning(lcPhonemeViewModelContextData)
                << "Failed to copy the complete original phoneme sequence";
            return nullptr;
        }
        for (qsizetype index = 0; index < originalPhonemes.size(); ++index) {
            auto *original = originalPhonemes.at(index);
            replacements.append({
                original,
                editedPhonemes.at(index),
                phonemeViewItemMap.value(original),
            });
        }

        binding->activeSequence = editedSequence;
        binding->edited = true;
        for (const auto &replacement : replacements) {
            auto *viewItem = replacement.viewItem;
            if (!viewItem) {
                continue;
            }
            disconnect(replacement.original, nullptr, this, nullptr);
            phonemeViewItemMap.remove(replacement.original);
            phonemeDocumentItemMap.insert(viewItem, replacement.edited);
            phonemeViewItemMap.insert(replacement.edited, viewItem);
            connectActivePhoneme(replacement.edited, viewItem, false);
            ++syncingViewDepth;
            viewItem->setEdited(true);
            --syncingViewDepth;
        }
        return phonemeDocumentItemMap.value(target);
    }

    void PhonemeViewModelContextData::restoreTargetViewItem() {
        auto *viewItem = targetViewItem.data();
        auto *phoneme = phonemeDocumentItemMap.value(viewItem);
        auto *binding = bindingForViewItem(viewItem);
        if (!viewItem || !phoneme || !binding) {
            return;
        }
        ++syncingViewDepth;
        viewItem->setPosition(originalViewPosition);
        viewItem->setContent(originalViewContent);
        viewItem->setEdited(binding->edited);
        --syncingViewDepth;
        updateLiveOrder(binding, viewItem);
        updateNoteInterval(binding);
    }

    sflow::PhonemeSequenceInteractionController *
    PhonemeViewModelContextData::createController(QObject *parent) {
        auto *controller = new sflow::PhonemeSequenceInteractionController(parent);
        controller->setPrimaryItemInteraction(
            sflow::PhonemeSequenceInteractionController::Move);
        controller->setSecondaryItemInteraction(
            sflow::PhonemeSequenceInteractionController::None);

        connect(controller, &sflow::PhonemeSequenceInteractionController::movingStarted,
                this, [this](QQuickItem *sequenceItem,
                             sflow::PhonemeViewModel *viewItem) {
                    beginEdit(MoveEdit, sequenceItem, viewItem);
                });
        connect(controller, &sflow::PhonemeSequenceInteractionController::movingCommitted,
                this, [this](QQuickItem *sequenceItem,
                             sflow::PhonemeViewModel *viewItem) {
                    if (editOperation == MoveEdit &&
                        targetViewItem == viewItem) {
                        if (targetSequenceItem == sequenceItem) {
                            commitMove();
                        } else {
                            abortEdit();
                        }
                    } else if (auto *binding = bindingForViewItem(viewItem)) {
                        auto *phoneme = phonemeDocumentItemMap.value(viewItem);
                        ++syncingViewDepth;
                        viewItem->setPosition(
                            documentPositionToViewPosition(binding->note, phoneme->start()));
                        --syncingViewDepth;
                        updateLiveOrder(binding, viewItem);
                        updateNoteInterval(binding);
                    }
                });
        connect(controller, &sflow::PhonemeSequenceInteractionController::movingAborted,
                this, [this](QQuickItem *,
                             sflow::PhonemeViewModel *viewItem) {
                    if (editOperation == MoveEdit &&
                        targetViewItem == viewItem) {
                        abortEdit();
                    } else if (auto *binding = bindingForViewItem(viewItem)) {
                        auto *phoneme = phonemeDocumentItemMap.value(viewItem);
                        ++syncingViewDepth;
                        viewItem->setPosition(
                            documentPositionToViewPosition(binding->note, phoneme->start()));
                        --syncingViewDepth;
                        updateLiveOrder(binding, viewItem);
                        updateNoteInterval(binding);
                    }
                });
        connect(controller, &sflow::PhonemeSequenceInteractionController::itemDoubleClicked,
                this, [this](QQuickItem *sequenceItem,
                             sflow::PhonemeViewModel *viewItem) {
                    if (!beginEdit(TokenEdit, sequenceItem, viewItem)) {
                        return;
                    }
                    if (!QMetaObject::invokeMethod(
                            sequenceItem, "editInPlace",
                            Q_ARG(sflow::PhonemeViewModel *, viewItem))) {
                        abortEdit();
                    }
                });
        connect(controller,
                &sflow::PhonemeSequenceInteractionController::inPlaceEditOperationTriggered,
                this, [this](QQuickItem *sequenceItem,
                             sflow::PhonemeViewModel *viewItem,
                             sflow::PhonemeSequenceInteractionController::InPlaceEditOperation operation) {
                    switch (operation) {
                        case sflow::PhonemeSequenceInteractionController::StartEditing:
                            if (editOperation == NoEdit) {
                                beginEdit(TokenEdit, sequenceItem, viewItem);
                            }
                            break;
                        case sflow::PhonemeSequenceInteractionController::CommitEditing:
                            if (editOperation == TokenEdit &&
                                targetViewItem == viewItem) {
                                if (targetSequenceItem == sequenceItem) {
                                    commitTokenEdit();
                                } else {
                                    abortEdit();
                                }
                            }
                            break;
                        case sflow::PhonemeSequenceInteractionController::AbortEditing:
                            if (editOperation == TokenEdit &&
                                targetViewItem == viewItem) {
                                abortEdit();
                            }
                            break;
                    }
                });
        return controller;
    }

}

#include "moc_PhonemeViewModelContextData_p.cpp"
