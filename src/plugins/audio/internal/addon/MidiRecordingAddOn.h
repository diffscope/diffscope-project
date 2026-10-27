// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_MIDIRECORDINGADDON_H
#define DIFFSCOPE_AUDIO_MIDIRECORDINGADDON_H

#include <memory>

#include <QList>
#include <QPointer>
#include <QSet>
#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

#include <TalcsMidi/MidiInputEvent.h>

#include <transactional/TransactionController.h>

#include <audio/ProjectAudioContext.h>

class QTimer;

namespace Core {
    class DspxDocument;
    class ProjectTimeline;
    class ProjectWindowInterface;
}

namespace dspx {
    class Note;
    class NoteSequence;
}

namespace Audio {
    class MidiInputContext;
}

namespace Audio::Internal {

    class PlaybackAddOn;

    class MidiRecordingAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(bool recording READ isRecording NOTIFY stateChanged)
        Q_PROPERTY(bool paused READ isPaused NOTIFY stateChanged)
        Q_PROPERTY(bool busy READ isBusy NOTIFY stateChanged)
        Q_PROPERTY(bool canRecord READ canRecord NOTIFY canRecordChanged)

    public:
        explicit MidiRecordingAddOn(QObject *parent = nullptr);
        ~MidiRecordingAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

        static MidiRecordingAddOn *of(Core::ProjectWindowInterface *window);
        static MidiRecordingAddOn *activeRecording();

        bool isRecording() const;
        bool isPaused() const;
        bool isBusy() const;
        bool canRecord() const;

        enum Decision {
            Discard,
            Quantize,
            Keep,
        };
        Q_ENUM(Decision)

        Q_INVOKABLE QObject *getRecordAction() const;
        Q_INVOKABLE void startRecording();
        void pauseRecording();
        void stopRecording();
        Q_INVOKABLE void finishRecording(Decision decision, int denominator = 8, int tuplet = 2, bool trimOverlaps = true, QObject *dialog = nullptr);

    Q_SIGNALS:
        void stateChanged();
        void canRecordChanged();

    protected:
        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        class InputListener;
        friend class InputListener;
        friend class PlaybackAddOn;

        enum State {
            Idle,
            Starting,
            Recording,
            Pausing,
            RecordingPaused,
            Stopping,
            AwaitingQuantization,
            Finishing,
        };

        struct HeldNote {
            quint64 sourceId;
            int channel;
            int key;
            QPointer<dspx::Note> note;
        };

        dspx::NoteSequence *currentNoteSequence() const;
        bool isCapturing() const;
        void beginCapture(bool continuePlayback = false);
        void resumeCapture();
        bool suspendCapture(State state);
        void repositionRecording(int position);
        void showQuantizationDialog();
        void requestPlayback(PlaybackAddOn *playback, quint64 serial);
        void cancelPendingPlayback(PlaybackAddOn *playback = nullptr);
        bool handlePlaybackStatusChange(ProjectAudioContext::PlaybackStatus status);
        void setPlaybackStatus(ProjectAudioContext::PlaybackStatus status);
        void setState(State state);
        void notifyOtherRecorders();
        void consumeInput(double until, bool stop = false);
        void processEvent(const talcs::MidiInputEvent &event);
        int tickAt(double time) const;
        void advance(double time);
        void growNotes(int tick);
        void finalizeNote(dspx::Note *note, int tick);
        void trimRecordedNoteOverlaps();
        void finishHeldNotes(int tick, quint64 sourceId = 0, int channel = 0, bool allSources = true);
        void failRecording(const char *message);

        QPointer<Core::DspxDocument> m_document;
        Core::ProjectTimeline *m_timeline{};
        QPointer<MidiInputContext> m_inputContext;
        QPointer<ProjectAudioContext> m_audioContext;
        std::unique_ptr<InputListener> m_listener;
        QPointer<dspx::NoteSequence> m_noteSequence;
        Core::TransactionController::TransactionId m_transactionId{Core::TransactionController::TransactionId::Invalid};
        QList<HeldNote> m_heldNotes;
        QList<QPointer<dspx::Note>> m_recordedNotes;
        QSet<dspx::Note *> m_recordedNoteSet;
        QList<talcs::MidiInputEvent> m_pendingEvents;
        QTimer *m_timer{};
        QPointer<QObject> m_dialog;
        QPointer<PlaybackAddOn> m_pendingPlayback;
        quint64 m_pendingPlaybackSerial{};
        State m_state{Idle};
        double m_startTime{};
        double m_startMillisecond{};
        double m_clockSampleRate{};
        int m_frozenPosition{};
        bool m_usesAudioClock{};
        bool m_changed{};
        bool m_updatingPlayback{};
        bool m_updatingPlayhead{};
        bool m_closePending{};
        bool m_stopRequested{};
    };

}

#endif // DIFFSCOPE_AUDIO_MIDIRECORDINGADDON_H
