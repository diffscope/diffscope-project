// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H
#define DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H

#include <functional>
#include <memory>

#include <QFuture>
#include <QMutex>
#include <QObject>
#include <QString>
#include <qqmlintegration.h>

#include <audio/audioglobal.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class SingingClip;
}

namespace talcs {
    class AudioSource;
    class MixerAudioSource;
    class PositionableMixerAudioSource;
    class TransportAudioSource;
}

namespace Audio {

    namespace Internal {
        class ProjectAudioAddOn;
    }

    class ProjectAudioContextPrivate;
    class ProjectAudioContext;
    class ProjectAudioRenderSessionPrivate;

    /** The format and preparation policy for an exclusive render session. */
    struct ProjectAudioRenderConfig {
        enum class Mode {
            Realtime, /**< Skips sources that are not ready. */
            Offline, /**< Requires source preparation before block rendering. */
        };

        /** Maximum output channel count supported by the mixer's channel mute mask. */
        static constexpr int MaximumChannels = 32;

        /** The maximum number of frames in a block. Must be positive. */
        qint64 maximumFrames = 0;
        /** The sample rate in Hz. Must be finite and positive. */
        double sampleRate = 0;
        /** The output channel count. Must be one or an even count up to MaximumChannels. */
        int channels = 2;
        Mode mode = Mode::Realtime;
    };

    /** The outcome of source preparation and its optional diagnostic detail. */
    struct RenderPreparationResult {
        enum Code { Ready, Failed, Cancelled, SourceUnavailable };

        Code code = Ready;
        QString detail;
    };

    /** Prepares sources for offline rendering in a single project. */
    class AUDIO_EXPORT RenderPreparationProvider : public QObject {
    public:
        using QObject::QObject;
        ~RenderPreparationProvider() override = default;

        /** Starts preparation on the project thread and returns a future for its result. Document edits may continue while it runs. */
        virtual QFuture<RenderPreparationResult> prepare(const ProjectAudioRenderConfig &config) = 0;
        /** Returns whether the sources are ready for config. Called with the document read lock held. */
        virtual bool isReady(const ProjectAudioRenderConfig &config) const = 0;
    };

    /** Owns exclusive access to a project's mix graph. Manage the session on the project thread and render blocks on the render thread. */
    class AUDIO_EXPORT ProjectAudioRenderSession {
    public:
        /** Releases the session if it is still active. */
        ~ProjectAudioRenderSession();

        /**
         * Configures the session once. Returns true on success, or false if the session is already configured,
         * has ended, the format is invalid, or the mix graph could not be opened.
         * This configuration selects the project mix for one or two output channels and per-track stereo pairs
         * for larger channel counts.
         */
        bool configure(const ProjectAudioRenderConfig &config);
        /**
         * Returns a future for the preparation result. A configured realtime session is immediately ready.
         * Successful offline preparation holds a document read lock until release().
         */
        QFuture<RenderPreparationResult> prepare();
        /**
         * Renders frames at the absolute sample position into caller-owned planar buffers.
         * Returns frames on success or zero without writing output if the session is unready or the range is invalid.
         * @pre channels provides one writable array of at least frames samples per configured channel.
         */
        qint64 renderBlock(qint64 position, float *const *channels, qint64 frames);
        /**
         * Renders frames at the absolute sample position if the registered sources are ready for that range.
         * Returns frames on success or zero without advancing the render position or writing output if the
         * range is unavailable, the graph is being updated, or renderBlock() would reject the request.
         * Does not wait for source preparation. Uses the same buffer preconditions as renderBlock().
         */
        qint64 renderBlockIfReady(qint64 position, float *const *channels, qint64 frames);
        /**
         * Renders through the project's local playback graph at its transport position.
         * Returns frames on success or zero if playback is stopped, the range is unready, or the graph is being updated.
         * Uses the same buffer preconditions as renderBlock().
         */
        qint64 renderPlaybackBlockIfReady(float *const *channels, qint64 frames);
        /**
         * Cancels preparation and returns the mix graph after any in-progress read completes.
         * Returns true if already released or if graph restoration succeeds, and false if restoration fails.
         */
        bool release();

    private:
        friend class ProjectAudioContext;
        friend class ProjectAudioContextPrivate;
        explicit ProjectAudioRenderSession(ProjectAudioContext *context);
        Q_DISABLE_COPY_MOVE(ProjectAudioRenderSession)

        qint64 renderBlockInternal(qint64 position, float *const *channels, qint64 frames, bool requireReadyRange);

        QScopedPointer<ProjectAudioRenderSessionPrivate> d_ptr;
    };

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

        enum class PendingSourcePolicy {
            BufferTransport, /**< Pauses the local transport while waiting for sources. */
            SkipUnavailable, /**< Skips sources that are not ready. */
            RequireReady, /**< Requires preparation before rendering. */
        };
        Q_ENUM(PendingSourcePolicy)

        PlaybackStatus status() const;
        /**
         * Requests a status change from the local project transport.
         * An active recording may defer playback until its transaction is resolved.
         */
        void setStatus(PlaybackStatus status);
        /**
         * Reserves playback for the external controller, including periods without a render session.
         * Pass nullptr to release the reservation.
         */
        void setExternalPlaybackController(QObject *controller);
        /** Transfers playback control to or from the project transport during an external realtime render session. */
        void setLocalPlaybackEnabled(bool enabled);
        /** Returns whether playback or recording is running. */
        bool isRunning() const;
        /** Returns whether recording is running or paused. */
        bool isRecording() const;
        /** Returns whether playback or recording is paused. */
        bool isPaused() const;
        /** Returns whether a render session currently owns the mix graph. */
        bool hasRenderSession() const;
        /** Returns the current sample rate of the project mix graph in Hz. */
        double sampleRate() const;
        /** Returns the current policy for sources that are not ready. */
        PendingSourcePolicy pendingSourcePolicy() const;
        /** Reserves the mix graph. Returns an unconfigured session on success or nullptr if access is unavailable. */
        std::unique_ptr<ProjectAudioRenderSession> acquireRenderSession();
        /**
         * Reserves the mix graph for external block rendering with the project's output routing.
         * Returns a configured session on success or nullptr if acquisition or configuration fails.
         */
        std::unique_ptr<ProjectAudioRenderSession> acquireRenderSession(const ProjectAudioRenderConfig &config);
        /** Registers a provider without taking ownership. Destruction of the provider unregisters it. */
        void registerRenderPreparationProvider(RenderPreparationProvider *provider);
        /**
         * Registers a provider on the project thread with an optional readiness predicate for sample ranges.
         * The predicate returns whether the range [position, position + frames) is ready at the current sample rate.
         * It runs on the render thread under the graph lock and must not wait for preparation or call into the
         * project thread or render session. Captured state must remain valid until unregistration.
         */
        void registerRenderPreparationProvider(RenderPreparationProvider *provider, std::function<bool(qint64 position, qint64 frames)> rangeReady);
        /** Unregisters a provider. An active external offline session is released if the provider was registered. */
        void unregisterRenderPreparationProvider(RenderPreparationProvider *provider);
        /**
         * Locks source and readiness updates against renderBlockIfReady(). Hold the returned guard across both
         * updates on the project thread so the readiness predicate and the graph describe the same sources.
         */
        [[nodiscard]]
        QMutexLocker<QRecursiveMutex> lockRenderGraphForUpdate();
        /**
         * Updates the displayed playback state and timeline position under external playback control.
         * position is measured in samples at the current rate. Local playback and recording states are ignored.
         */
        void setExternalPlaybackState(PlaybackStatus status, qint64 position);

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
        /** Notifies that a render session was acquired or released. */
        void renderSessionChanged();
        /** Reports the effective sample rate after session configuration or release. */
        void sampleRateChanged(double sampleRate);
        /** Reports a change in the policy for sources that are not ready. */
        void pendingSourcePolicyChanged(PendingSourcePolicy policy);
        void playbackSuppressedClipChanged(dspx::SingingClip *clip);
        /** Notifies extensions before the MIDI recording quantization dialog opens. The initialDenominator and tuplet properties configure its initial grid. */
        void midiRecordingQuantizationDialogCreated(QObject *dialog);

    private:
        friend class ProjectAudioContextPrivate;
        friend class ProjectAudioRenderSession;
        friend class ProjectAudioRenderSessionPrivate;
        friend class Internal::ProjectAudioAddOn;
        explicit ProjectAudioContext(Core::ProjectWindowInterface *windowHandle);

        QScopedPointer<ProjectAudioContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H
