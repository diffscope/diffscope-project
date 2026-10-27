// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MidiRecordingAddOn.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <QEvent>
#include <QLoggingCategory>
#include <QMutex>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QScopedValueRollback>
#include <QTimer>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/windowsystem.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <TalcsCore/TransportAudioSource.h>
#include <TalcsDevice/AudioDevice.h>
#include <TalcsMidi/MidiMessageListener.h>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DefaultLyricManager.h>
#include <coreplugin/DocumentEditScenario.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/GlobalAudioContext.h>
#include <audio/MidiInputContext.h>
#include <audio/ProjectAudioContext.h>
#include <audio/internal/EffectsAddOn.h>
#include <audio/internal/PlaybackAddOn.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcMidiRecording, "diffscope.audio.midirecording")

    namespace {

        class RecordingDialogScenario : public Core::DocumentEditScenario {
        public:
            QObject *createDialog(MidiRecordingAddOn *addOn) const {
                QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "MidiRecordingDialog");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                return createAndPositionDialog(&component, {{QStringLiteral("addOn"), QVariant::fromValue(addOn)}});
            }
        };

    }

    class MidiRecordingAddOn::InputListener : public talcs::MidiMessageListener {
    public:
        struct Batch {
            QList<talcs::MidiInputEvent> events;
            bool overflow;
        };

        explicit InputListener(MidiRecordingAddOn *owner) : m_owner(owner) {
        }

        void start() {
            QMutexLocker locker(&m_mutex);
            m_events.clear();
            m_overflow = false;
            m_accepting = true;
        }

        Batch takeEvents(bool stop) {
            QMutexLocker locker(&m_mutex);
            if (stop)
                m_accepting = false;
            return {std::exchange(m_events, {}), std::exchange(m_overflow, false)};
        }

    protected:
        bool processEvent(const talcs::MidiInputEvent &event) override {
            const auto &message = event.message;
            if (event.type == talcs::MidiInputEvent::Message && !message.isNoteOnOrOff() && !message.isAllNotesOff() && !message.isAllSoundOff())
                return false;
            bool notify;
            {
                QMutexLocker locker(&m_mutex);
                if (!m_accepting)
                    return false;
                if (m_events.size() >= 4096) {
                    m_overflow = true;
                    return false;
                }
                notify = m_events.isEmpty();
                m_events.append(event);
            }
            if (notify) {
                QMetaObject::invokeMethod(m_owner, [owner = m_owner] {
                    if (owner->isCapturing())
                        owner->consumeInput(talcs::MidiInputEvent::currentTime());
                }, Qt::QueuedConnection);
            }
            return false;
        }

    private:
        MidiRecordingAddOn *m_owner;
        QMutex m_mutex;
        QList<talcs::MidiInputEvent> m_events;
        bool m_accepting{};
        bool m_overflow{};
    };

    MidiRecordingAddOn::MidiRecordingAddOn(QObject *parent)
        : WindowInterfaceAddOn(parent), m_listener(std::make_unique<InputListener>(this)) {
    }

    MidiRecordingAddOn::~MidiRecordingAddOn() {
        const auto wasBusy = isBusy();
        m_state = Finishing;
        if (m_timer)
            m_timer->stop();
        m_listener->takeEvents(true);
        if (m_inputContext)
            m_inputContext->listener()->removeFilter(m_listener.get());
        if (m_audioContext && m_transactionId != Core::TransactionController::TransactionId::Invalid) {
            setPlaybackStatus(ProjectAudioContext::Stopped);
            m_audioContext->setPlaybackSuppressedClip(nullptr);
        }
        if (m_document && m_transactionId != Core::TransactionController::TransactionId::Invalid && !m_document->transactionController()->abortTransaction(m_transactionId))
            qCCritical(lcMidiRecording) << "The MIDI recording transaction could not be discarded.";
        m_state = Idle;
        if (wasBusy)
            notifyOtherRecorders();
    }

    void MidiRecordingAddOn::initialize() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        window->addObject(this);
        m_document = window->projectDocumentContext()->document();
        m_audioContext = ProjectAudioContext::of(window);
        m_timeline = window->projectTimeline();
        m_timer = new QTimer(this);
        m_timer->setTimerType(Qt::PreciseTimer);
        m_timer->setInterval(16);
        connect(m_timer, &QTimer::timeout, this, [this] {
            const auto now = talcs::MidiInputEvent::currentTime();
            consumeInput(now);
            if (isCapturing())
                advance(now);
        });
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "MidiRecordingActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{QStringLiteral("addOn"), QVariant::fromValue(this)}});
        if (!object) {
            qFatal() << component.errorString();
        }
        object->setParent(this);
        QMetaObject::invokeMethod(object, "registerToContext", window->actionContext());
        window->window()->installEventFilter(this);
    }

    void MidiRecordingAddOn::extensionsInitialized() {
        m_inputContext = MidiInputContext::instance();
        m_inputContext->listener()->addFilter(m_listener.get());
        connect(m_document->selectionModel()->noteSelectionModel(), &dspx::NoteSelectionModel::noteSequenceWithSelectedItemsChanged, this, [this] {
            Q_EMIT canRecordChanged();
            if (isRecording() && currentNoteSequence() != m_noteSequence)
                stopRecording();
        }, Qt::QueuedConnection);
        connect(m_timeline, &Core::ProjectTimeline::positionChanged, this, [this](int position) {
            if (isRecording() && !m_updatingPlayhead)
                repositionRecording(position);
        });
        const auto pauseForOutputChange = [this] {
            if (isCapturing() && !m_updatingPlayback)
                pauseRecording();
        };
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::deviceChanged, this, pauseForOutputChange);
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::sampleRateChanged, this, pauseForOutputChange);
        connect(m_document->transactionController(), &Core::TransactionController::transactionActiveChanged, this, [this](bool active) {
            if (!active && isBusy() && m_state != Finishing && m_transactionId != Core::TransactionController::TransactionId::Invalid)
                failRecording("The MIDI recording transaction is no longer active.");
        });
        Q_EMIT canRecordChanged();
    }

    MidiRecordingAddOn *MidiRecordingAddOn::of(Core::ProjectWindowInterface *window) {
        return window->getFirstObject<MidiRecordingAddOn>();
    }

    MidiRecordingAddOn *MidiRecordingAddOn::activeRecording() {
        for (auto window : Core::CoreInterface::windowSystem()->windows()) {
            if (auto recorder = window->getFirstObject<MidiRecordingAddOn>(); recorder && recorder->isBusy())
                return recorder;
        }
        return nullptr;
    }

    bool MidiRecordingAddOn::isRecording() const {
        return m_audioContext && m_audioContext->isRecording();
    }

    bool MidiRecordingAddOn::isPaused() const {
        return m_audioContext && m_audioContext->status() == ProjectAudioContext::RecordingPaused;
    }

    bool MidiRecordingAddOn::isCapturing() const {
        return m_state == Recording;
    }

    bool MidiRecordingAddOn::isBusy() const {
        return m_state != Idle;
    }

    dspx::NoteSequence *MidiRecordingAddOn::currentNoteSequence() const {
        return m_document ? m_document->selectionModel()->noteSelectionModel()->noteSequenceWithSelectedItems() : nullptr;
    }

    bool MidiRecordingAddOn::canRecord() const {
        const auto sequence = currentNoteSequence();
        return !isBusy() && !activeRecording() && m_inputContext && m_audioContext && sequence && sequence->singingClip()->clipSequence();
    }

    void MidiRecordingAddOn::setState(State state) {
        if (m_state == state)
            return;
        const auto wasBusy = isBusy();
        const char *names[] = {"idle", "starting", "recording", "pausing", "recordingPaused", "stopping", "awaitingQuantization", "finishing"};
        qCInfo(lcMidiRecording) << "Leaving recording state." << names[m_state];
        m_state = state;
        qCInfo(lcMidiRecording) << "Entering recording state." << names[m_state];
        switch (state) {
        case Recording:
            setPlaybackStatus(ProjectAudioContext::Recording);
            break;
        case RecordingPaused:
            setPlaybackStatus(ProjectAudioContext::RecordingPaused);
            break;
        case AwaitingQuantization:
            setPlaybackStatus(ProjectAudioContext::Stopped);
            break;
        default:
            break;
        }
        Q_EMIT stateChanged();
        Q_EMIT canRecordChanged();
        if (wasBusy != isBusy())
            notifyOtherRecorders();
    }

    void MidiRecordingAddOn::notifyOtherRecorders() {
        for (auto window : Core::CoreInterface::windowSystem()->windows()) {
            if (auto recorder = window->getFirstObject<MidiRecordingAddOn>(); recorder && recorder != this)
                Q_EMIT recorder->canRecordChanged();
        }
    }

    QObject *MidiRecordingAddOn::getRecordAction() const {
        const auto context = windowHandle()->cast<Core::ProjectWindowInterface>()->actionContext();
        const auto id = QStringLiteral("org.diffscope.audio.playback.record");
        const auto component = context->action(id);
        Q_ASSERT(component);
        const auto object = component->create(component->creationContext());
        Q_ASSERT(object);
        context->attachActionInfo(id, object);
        return object;
    }

    void MidiRecordingAddOn::startRecording() {
        if (m_state == RecordingPaused) {
            resumeCapture();
            return;
        }
        if (isBusy())
            return;
        if (activeRecording()) {
            qCCritical(lcMidiRecording) << "MIDI recording is already active in another project window.";
            Q_EMIT stateChanged();
            return;
        }
        if (!canRecord()) {
            Q_EMIT stateChanged();
            return;
        }
        m_noteSequence = currentNoteSequence();
        m_changed = false;
        m_stopRequested = false;
        m_heldNotes.clear();
        m_recordedNotes.clear();
        m_recordedNoteSet.clear();
        m_pendingEvents.clear();
        cancelPendingPlayback();
        setState(Starting);
        const auto transaction = m_document->transactionController()->beginTransaction();
        if (transaction == Core::TransactionController::TransactionId::Invalid) {
            qCCritical(lcMidiRecording) << "The MIDI recording transaction could not start.";
            m_noteSequence.clear();
            setState(Idle);
            return;
        }
        m_transactionId = transaction;
        if (!m_noteSequence || m_noteSequence != currentNoteSequence()) {
            failRecording("The MIDI recording note sequence is unavailable.");
            return;
        }
        const auto continuingPlayback = m_audioContext->isRunning();
        for (auto window : Core::CoreInterface::windowSystem()->windows()) {
            if (auto projectWindow = qobject_cast<Core::ProjectWindowInterface *>(window)) {
                const auto context = ProjectAudioContext::of(projectWindow);
                if (auto playback = PlaybackAddOn::of(projectWindow)) {
                    playback->cancelPendingPlayback();
                    if (projectWindow != windowHandle() && context && context->isRunning())
                        playback->pause();
                }
            }
        }
        m_audioContext->setPlaybackSuppressedClip(m_noteSequence->singingClip());
        m_frozenPosition = m_timeline->position();
        if (!continuingPlayback)
            m_timeline->setLastPosition(m_timeline->position());
        beginCapture(continuingPlayback);
        if (m_closePending || m_pendingPlayback)
            stopRecording();
    }

    void MidiRecordingAddOn::beginCapture(bool continuePlayback) {
        m_startMillisecond = m_timeline->musicTimeline()->create(0, 0, m_frozenPosition).millisecond();
        m_clockSampleRate = GlobalAudioContext::sampleRate();
        if (continuePlayback && m_clockSampleRate > 0) {
            m_startMillisecond = m_audioContext->transport()->position() * 1000.0 / m_clockSampleRate;
            m_frozenPosition = m_timeline->musicTimeline()->create(m_startMillisecond).totalTick();
            QScopedValueRollback updating(m_updatingPlayhead, true);
            m_timeline->setPosition(m_frozenPosition);
        } else {
            if (auto effects = EffectsAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()))
                effects->refreshAllEffects();
            m_audioContext->transport()->setPosition(static_cast<qint64>(m_startMillisecond * m_clockSampleRate / 1000.0));
        }
        m_startTime = talcs::MidiInputEvent::currentTime();
        m_pendingEvents.clear();
        m_listener->start();
        setState(Recording);
        if (!isCapturing())
            return;
        const auto device = GlobalAudioContext::device();
        m_usesAudioClock = device && device->isOpen() && device->isStarted() && m_clockSampleRate > 0;
        if (isCapturing())
            m_timer->start();
    }

    bool MidiRecordingAddOn::suspendCapture(State state) {
        if (!isCapturing())
            return false;
        setState(state);
        m_timer->stop();
        const auto now = talcs::MidiInputEvent::currentTime();
        consumeInput(now, true);
        if (m_state != state)
            return false;
        if (!m_noteSequence) {
            failRecording("The MIDI recording note sequence is unavailable.");
            return false;
        }
        m_frozenPosition = tickAt(now);
        growNotes(m_frozenPosition);
        {
            QScopedValueRollback updating(m_updatingPlayhead, true);
            m_timeline->setPosition(m_frozenPosition);
        }
        finishHeldNotes(m_frozenPosition);
        return true;
    }

    void MidiRecordingAddOn::pauseRecording() {
        cancelPendingPlayback();
        if (!suspendCapture(Pausing))
            return;
        setState(RecordingPaused);
    }

    void MidiRecordingAddOn::resumeCapture() {
        if (m_state != RecordingPaused)
            return;
        if (!m_noteSequence || currentNoteSequence() != m_noteSequence) {
            stopRecording();
            return;
        }
        cancelPendingPlayback();
        beginCapture();
    }

    void MidiRecordingAddOn::repositionRecording(int position) {
        const auto capturing = isCapturing();
        if (capturing && !suspendCapture(Pausing))
            return;
        if (!capturing && m_state != RecordingPaused)
            return;
        m_frozenPosition = position;
        {
            QScopedValueRollback updating(m_updatingPlayhead, true);
            m_timeline->setPosition(position);
        }
        const auto millisecond = m_timeline->musicTimeline()->create(0, 0, position).millisecond();
        m_audioContext->transport()->setPosition(static_cast<qint64>(millisecond * GlobalAudioContext::sampleRate() / 1000.0));
        if (capturing)
            beginCapture();
    }

    void MidiRecordingAddOn::requestPlayback(PlaybackAddOn *playback, quint64 serial) {
        if (m_state == Finishing)
            return;
        m_pendingPlayback = playback;
        m_pendingPlaybackSerial = serial;
        stopRecording();
    }

    void MidiRecordingAddOn::cancelPendingPlayback(PlaybackAddOn *playback) {
        if (!playback || m_pendingPlayback == playback) {
            m_pendingPlayback.clear();
            m_pendingPlaybackSerial = 0;
        }
    }

    bool MidiRecordingAddOn::handlePlaybackStatusChange(ProjectAudioContext::PlaybackStatus status) {
        if (m_updatingPlayback)
            return false;
        if (status == ProjectAudioContext::Recording) {
            startRecording();
            return true;
        }
        if (status == ProjectAudioContext::RecordingPaused) {
            if (isCapturing())
                pauseRecording();
            return true;
        }
        if (!isBusy())
            return false;
        switch (status) {
        case ProjectAudioContext::Playing:
            PlaybackAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>())->play();
            break;
        case ProjectAudioContext::Paused:
            cancelPendingPlayback();
            if (isCapturing())
                pauseRecording();
            break;
        case ProjectAudioContext::Stopped:
            cancelPendingPlayback();
            m_stopRequested = true;
            stopRecording();
            break;
        case ProjectAudioContext::Recording:
        case ProjectAudioContext::RecordingPaused:
            break;
        }
        return true;
    }

    void MidiRecordingAddOn::setPlaybackStatus(ProjectAudioContext::PlaybackStatus status) {
        if (!m_audioContext)
            return;
        QScopedValueRollback updating(m_updatingPlayback, true);
        if (m_audioContext->status() == status) {
            if (auto playback = PlaybackAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()))
                playback->handlePlaybackStatusChanged(status);
        } else {
            m_audioContext->setStatus(status);
        }
    }

    int MidiRecordingAddOn::tickAt(double time) const {
        auto millisecond = m_startMillisecond + std::max(0.0, time - m_startTime) * 1000.0;
        const auto device = GlobalAudioContext::device();
        const auto sampleRate = m_clockSampleRate;
        if (m_audioContext && m_usesAudioClock && sampleRate > 0) {
            const auto transport = m_audioContext->transport();
            const auto advancing = device && device->isOpen() && device->isStarted() && transport->bufferingCounter() == 0;
            const auto offset = advancing ? time - talcs::MidiInputEvent::currentTime() : 0.0;
            millisecond = std::max(m_startMillisecond, transport->position() * 1000.0 / sampleRate + offset * 1000.0);
        }
        return m_timeline->musicTimeline()->create(millisecond).totalTick();
    }

    void MidiRecordingAddOn::consumeInput(double until, bool stop) {
        if (!isCapturing() && m_state != Pausing && m_state != Stopping)
            return;
        const auto state = m_state;
        auto batch = m_listener->takeEvents(stop);
        if (batch.overflow) {
            failRecording("The MIDI recording input queue overflowed.");
            return;
        }
        m_pendingEvents.append(batch.events);
        if (m_pendingEvents.size() > 4096) {
            failRecording("The MIDI recording input queue overflowed.");
            return;
        }
        std::stable_sort(m_pendingEvents.begin(), m_pendingEvents.end(), [](const auto &left, const auto &right) {
            return left.message.getTimeStamp() < right.message.getTimeStamp();
        });
        const auto end = std::upper_bound(m_pendingEvents.cbegin(), m_pendingEvents.cend(), until, [](double time, const auto &event) {
            return time < event.message.getTimeStamp();
        });
        const auto count = end - m_pendingEvents.cbegin();
        const auto events = m_pendingEvents.sliced(0, count);
        m_pendingEvents.remove(0, count);
        for (const auto &event : events) {
            if (event.message.getTimeStamp() >= m_startTime)
                processEvent(event);
            if (m_state != state)
                break;
        }
        if (stop)
            m_pendingEvents.clear();
    }

    void MidiRecordingAddOn::processEvent(const talcs::MidiInputEvent &event) {
        if (!m_noteSequence) {
            failRecording("The MIDI recording note sequence is unavailable.");
            return;
        }
        const auto &message = event.message;
        const auto tick = tickAt(message.getTimeStamp());
        if (event.type != talcs::MidiInputEvent::Message) {
            m_pendingEvents.removeIf([&](const auto &pending) {
                return event.type == talcs::MidiInputEvent::AllSourcesStopped || pending.sourceId == event.sourceId;
            });
            finishHeldNotes(tick, event.sourceId, 0, event.type == talcs::MidiInputEvent::AllSourcesStopped);
        } else if (message.isAllNotesOff() || message.isAllSoundOff()) {
            finishHeldNotes(tick, event.sourceId, message.getChannel(), false);
        } else if (message.isNoteOn()) {
            const auto position = static_cast<qint64>(tick) - m_noteSequence->singingClip()->start();
            if (position < 0 || position >= std::numeric_limits<int>::max()) {
                m_heldNotes.append({event.sourceId, message.getChannel(), message.getNoteNumber(), nullptr});
                return;
            }
            auto note = m_document->model()->createNote();
            note->setPosition(static_cast<int>(position));
            note->setLength(0);
            note->setKeyNumber(message.getNoteNumber());
            note->setLyric(Core::CoreInterface::defaultLyricManager()->getDefaultLyricForSingingClip(m_noteSequence->singingClip()));
            if (!m_noteSequence->insertItem(note)) {
                m_document->model()->destroyItem(note);
                failRecording("A MIDI recording note could not be inserted.");
                return;
            }
            m_heldNotes.append({event.sourceId, message.getChannel(), message.getNoteNumber(), note});
            m_recordedNotes.append(note);
            m_recordedNoteSet.insert(note);
            m_changed = true;
        } else if (message.isNoteOff()) {
            const auto held = std::find_if(m_heldNotes.begin(), m_heldNotes.end(), [&](const auto &entry) {
                return entry.sourceId == event.sourceId && entry.channel == message.getChannel() && entry.key == message.getNoteNumber();
            });
            if (held != m_heldNotes.end()) {
                const auto note = held->note;
                m_heldNotes.erase(held);
                finalizeNote(note, tick);
            }
        }
    }

    void MidiRecordingAddOn::growNotes(int tick) {
        if (!m_noteSequence)
            return;
        for (const auto &held : m_heldNotes) {
            if (held.note && m_noteSequence->contains(held.note)) {
                const auto end = static_cast<qint64>(tick) - m_noteSequence->singingClip()->start();
                const auto length = static_cast<int>(std::clamp<qint64>(end - held.note->position(), 0, std::numeric_limits<int>::max() - held.note->position()));
                if (held.note->length() != length)
                    held.note->setLength(length);
            }
        }
    }

    void MidiRecordingAddOn::advance(double time) {
        const auto tick = tickAt(time);
        growNotes(tick);
        QScopedValueRollback updating(m_updatingPlayhead, true);
        m_timeline->setPosition(tick);
    }

    void MidiRecordingAddOn::finalizeNote(dspx::Note *note, int tick) {
        if (!note || !m_noteSequence || !m_noteSequence->contains(note))
            return;
        const auto end = static_cast<qint64>(tick) - m_noteSequence->singingClip()->start();
        note->setLength(static_cast<int>(std::clamp<qint64>(end - note->position(), 1, std::numeric_limits<int>::max() - note->position())));
        for (auto other : m_noteSequence->slice(note->position(), note->length())) {
            if (!m_recordedNoteSet.contains(other)) {
                m_noteSequence->removeItem(other);
                m_document->model()->destroyItem(other);
            }
        }
    }

    void MidiRecordingAddOn::trimRecordedNoteOverlaps() {
        if (!m_noteSequence)
            return;
        QList<dspx::Note *> notes;
        for (const auto &note : m_recordedNotes) {
            if (note && m_noteSequence->contains(note))
                notes.append(note);
        }
        std::stable_sort(notes.begin(), notes.end(), [](const auto left, const auto right) {
            return left->position() < right->position();
        });
        for (qsizetype i = 1; i < notes.size(); ++i) {
            const auto previous = notes.at(i - 1);
            const auto next = notes.at(i);
            if (previous->position() == next->position()) {
                m_recordedNoteSet.remove(previous);
                m_noteSequence->removeItem(previous);
                m_document->model()->destroyItem(previous);
            } else if (static_cast<qint64>(previous->position()) + previous->length() > next->position()) {
                previous->setLength(next->position() - previous->position());
            }
        }
    }

    void MidiRecordingAddOn::finishHeldNotes(int tick, quint64 sourceId, int channel, bool allSources) {
        QList<QPointer<dspx::Note>> notes;
        for (auto it = m_heldNotes.begin(); it != m_heldNotes.end();) {
            if (allSources || (it->sourceId == sourceId && (channel == 0 || it->channel == channel))) {
                notes.append(it->note);
                it = m_heldNotes.erase(it);
            } else {
                ++it;
            }
        }
        for (const auto &note : notes)
            finalizeNote(note, tick);
    }

    void MidiRecordingAddOn::stopRecording() {
        if (m_state == AwaitingQuantization) {
            showQuantizationDialog();
            return;
        }
        if (isCapturing()) {
            if (!suspendCapture(Stopping))
                return;
        } else if (m_state == RecordingPaused) {
            setState(Stopping);
        } else {
            return;
        }
        setState(AwaitingQuantization);
        showQuantizationDialog();
    }

    void MidiRecordingAddOn::showQuantizationDialog() {
        if (m_state != AwaitingQuantization)
            return;
        if (!m_dialog) {
            RecordingDialogScenario scenario;
            scenario.setWindow(static_cast<QQuickWindow *>(windowHandle()->window()));
            scenario.setDocument(m_document);
            m_dialog = scenario.createDialog(this);
            m_dialog->setParent(this);
            connect(m_dialog, &QObject::destroyed, this, [this] {
                if (m_state == AwaitingQuantization)
                    failRecording("The MIDI recording dialog is unavailable.");
            });
            Q_EMIT m_audioContext->midiRecordingQuantizationDialogCreated(m_dialog.data());
        }
        QMetaObject::invokeMethod(m_dialog.data(), "open");
        windowHandle()->window()->raise();
        windowHandle()->window()->requestActivate();
    }

    void MidiRecordingAddOn::finishRecording(Decision decision, int denominator, int tuplet, bool trimOverlaps, QObject *dialog) {
        if (m_state != AwaitingQuantization || !m_document)
            return;
        if (dialog && dialog != m_dialog.data())
            return;
        if (decision != Discard && decision != Quantize && decision != Keep)
            return;
        const auto controller = m_document->transactionController();
        const auto transactionActive = controller->isTransactionActive();
        if (!transactionActive)
            decision = Discard;
        const bool validDenominator = denominator >= 1 && denominator <= 128 && (denominator & (denominator - 1)) == 0;
        if (decision == Quantize && (!validDenominator || (tuplet != 2 && tuplet != 3 && tuplet != 5)))
            return;
        setState(Finishing);
        if (decision == Quantize && m_noteSequence) {
            const auto step = std::max(1, m_timeline->musicTimeline()->ticksPerQuarterNote() * 8 / denominator / tuplet);
            const auto offset = m_noteSequence->singingClip()->start();
            const auto quantize = [step](qint64 tick) {
                return static_cast<qint64>(std::llround(static_cast<double>(tick) / step)) * step;
            };
            for (const auto &note : m_recordedNotes) {
                if (!note || !m_noteSequence->contains(note))
                    continue;
                const auto start = std::clamp<qint64>(quantize(static_cast<qint64>(note->position()) + offset) - offset, 0, std::numeric_limits<int>::max() - step);
                const auto end = std::clamp<qint64>(quantize(static_cast<qint64>(note->position()) + note->length() + offset) - offset, 0, std::numeric_limits<int>::max());
                note->setPosition(static_cast<int>(start));
                note->setLength(static_cast<int>(std::max<qint64>(step, end - start)));
            }
        }
        if (decision != Discard && trimOverlaps)
            trimRecordedNoteOverlaps();
        const bool success = !transactionActive || (decision == Discard || !m_changed
                                                       ? controller->abortTransaction(m_transactionId)
                                                       : controller->commitTransaction(m_transactionId, tr("Recording MIDI")));
        if (!success) {
            qCCritical(lcMidiRecording) << "The MIDI recording transaction could not finish.";
            cancelPendingPlayback();
            setState(AwaitingQuantization);
            QMetaObject::invokeMethod(this, &MidiRecordingAddOn::showQuantizationDialog, Qt::QueuedConnection);
            return;
        }
        m_transactionId = Core::TransactionController::TransactionId::Invalid;
        m_noteSequence.clear();
        m_recordedNotes.clear();
        m_recordedNoteSet.clear();
        m_heldNotes.clear();
        m_pendingEvents.clear();
        if (m_audioContext)
            m_audioContext->setPlaybackSuppressedClip(nullptr);
        if (m_dialog) {
            disconnect(m_dialog.data(), nullptr, this, nullptr);
            QMetaObject::invokeMethod(m_dialog.data(), "close");
            m_dialog->deleteLater();
            m_dialog.clear();
        }
        setPlaybackStatus(m_stopRequested ? ProjectAudioContext::Stopped : ProjectAudioContext::Paused);
        if (auto playback = PlaybackAddOn::of(windowHandle()->cast<Core::ProjectWindowInterface>()); playback && !m_closePending)
            playback->finishRecordingPlayback(m_stopRequested);
        const auto pendingPlayback = std::exchange(m_pendingPlayback, {});
        const auto pendingSerial = std::exchange(m_pendingPlaybackSerial, 0);
        const auto closing = std::exchange(m_closePending, false);
        setState(Idle);
        if (pendingPlayback && (!closing || pendingPlayback->windowHandle() != windowHandle())) {
            QMetaObject::invokeMethod(pendingPlayback.data(), [pendingPlayback, pendingSerial] {
                pendingPlayback->playAfterRecording(pendingSerial);
            }, Qt::QueuedConnection);
        }
        if (closing) {
            QMetaObject::invokeMethod(windowHandle()->window(), &QWindow::close, Qt::QueuedConnection);
        }
    }

    void MidiRecordingAddOn::failRecording(const char *message) {
        qCCritical(lcMidiRecording) << message;
        setState(Finishing);
        m_timer->stop();
        m_listener->takeEvents(true);
        cancelPendingPlayback();
        setState(AwaitingQuantization);
        finishRecording(Discard);
    }

    bool MidiRecordingAddOn::eventFilter(QObject *object, QEvent *event) {
        if (object == windowHandle()->window() && event->type() == QEvent::Close && isBusy()) {
            event->ignore();
            m_closePending = true;
            stopRecording();
            return true;
        }
        return WindowInterfaceAddOn::eventFilter(object, event);
    }

}

#include "moc_MidiRecordingAddOn.cpp"
