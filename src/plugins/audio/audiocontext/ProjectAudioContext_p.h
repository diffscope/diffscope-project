// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_P_H
#define DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_P_H

#include <audio/ProjectAudioContext.h>

#include <atomic>
#include <functional>
#include <memory>

#include <QHash>
#include <QMetaObject>
#include <QPointer>

namespace talcs {
    class DspxProjectContext;
}

namespace Audio {

    class ProjectAudioContextPrivate {
        Q_DECLARE_PUBLIC(ProjectAudioContext)
    public:
        struct PreparationProvider {
            QMetaObject::Connection destructionConnection;
            std::function<bool(qint64, qint64)> rangeReady;
        };

        ProjectAudioContext *q_ptr{};
        Core::ProjectWindowInterface *windowHandle{};
        ProjectAudioContext::PlaybackStatus status{ProjectAudioContext::Stopped};
        // A true return value consumes the request before the playback status changes.
        std::function<bool(ProjectAudioContext::PlaybackStatus)> playbackStatusChangeHandler;
        QPointer<QObject> externalPlaybackController;
        bool localPlaybackEnabled = false;
        QPointer<dspx::SingingClip> playbackSuppressedClip;
        QMetaObject::Connection suppressedClipDestroyedConnection;
        std::unique_ptr<talcs::DspxProjectContext> projectContext;
        std::unique_ptr<talcs::AudioSource> deviceSource;
        talcs::MixerAudioSource *metronomeControlMixer{};
        ProjectAudioRenderSession *renderSession = nullptr;
        ProjectAudioContext::PendingSourcePolicy pendingSourcePolicy = ProjectAudioContext::PendingSourcePolicy::BufferTransport;
        QRecursiveMutex renderGraphMutex;
        QHash<RenderPreparationProvider *, PreparationProvider> preparationProviders;
        std::shared_ptr<std::atomic<quint64>> renderSeekGeneration = std::make_shared<std::atomic<quint64>>(0);

        void setPendingSourcePolicy(ProjectAudioContext::PendingSourcePolicy policy);
        void setPlaybackStatus(ProjectAudioContext::PlaybackStatus status);
        bool usesLocalTransport() const;
        bool hasRenderRoutingOverride() const;
        void releaseRenderSession();

        static ProjectAudioContext *create(Core::ProjectWindowInterface *windowHandle);
        static ProjectAudioContextPrivate *of(ProjectAudioContext *q);
    };

}

#endif // DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_P_H
