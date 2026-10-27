// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H
#define DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H

#include <QObject>
#include <qqmlintegration.h>

#include <audio/audioglobal.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class SingingClip;
}

namespace talcs {
    class MixerAudioSource;
    class PositionableMixerAudioSource;
    class TransportAudioSource;
}

namespace Audio {

    namespace Internal {
        class ProjectAudioAddOn;
    }

    class ProjectAudioContextPrivate;

    class AUDIO_EXPORT ProjectAudioContext : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ProjectAudioContext)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(PlaybackStatus status READ status WRITE setStatus NOTIFY statusChanged)
        Q_PROPERTY(bool running READ isRunning NOTIFY transportStateChanged)
        Q_PROPERTY(bool recording READ isRecording NOTIFY transportStateChanged)
        Q_PROPERTY(bool paused READ isPaused NOTIFY transportStateChanged)

    public:
        ~ProjectAudioContext() override;

        static ProjectAudioContext *of(Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;

        enum PlaybackStatus {
            Stopped,
            Playing,
            Paused,
            Recording,
            RecordingPaused,
        };
        Q_ENUM(PlaybackStatus)
        PlaybackStatus status() const;
        /** Requests a playback status. An active recording may defer playback until its transaction is resolved. */
        void setStatus(PlaybackStatus status);
        /** Returns whether playback or recording is running. */
        bool isRunning() const;
        /** Returns whether recording is running or paused. */
        bool isRecording() const;
        /** Returns whether playback or recording is paused. */
        bool isPaused() const;

        /** Returns the singing clip excluded from playback, or nullptr if all clips are enabled. */
        dspx::SingingClip *playbackSuppressedClip() const;
        /** Excludes a clip from mixing and buffering without changing its document mute setting. Pass nullptr to restore playback. */
        void setPlaybackSuppressedClip(dspx::SingingClip *clip);

        talcs::MixerAudioSource *preMixer() const;
        talcs::MixerAudioSource *metronomeControlMixer() const;
        talcs::TransportAudioSource *transport() const;
        talcs::PositionableMixerAudioSource *postMixer() const;
        talcs::PositionableMixerAudioSource *masterControlMixer() const;
        talcs::PositionableMixerAudioSource *masterTrackMixer() const;

    Q_SIGNALS:
        void statusChanged(PlaybackStatus status);
        void transportStateChanged();
        void playbackSuppressedClipChanged(dspx::SingingClip *clip);
        /** Notifies extensions before the MIDI recording quantization dialog opens. The initialDenominator and tuplet properties configure its initial grid. */
        void midiRecordingQuantizationDialogCreated(QObject *dialog);

    private:
        friend class ProjectAudioContextPrivate;
        friend class Internal::ProjectAudioAddOn;
        explicit ProjectAudioContext(Core::ProjectWindowInterface *windowHandle);

        QScopedPointer<ProjectAudioContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H
