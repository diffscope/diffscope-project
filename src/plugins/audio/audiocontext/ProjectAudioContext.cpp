// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectAudioContext.h"
#include "ProjectAudioContext_p.h"

#include <QDebug>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsDspx/DspxProjectContext.h>

#include <dspxmodelORM/SingingClip.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace Audio {

    ProjectAudioContext::ProjectAudioContext(Core::ProjectWindowInterface *windowHandle)
        : QObject(windowHandle), d_ptr(new ProjectAudioContextPrivate) {
        Q_D(ProjectAudioContext);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
        d->projectContext = std::make_unique<talcs::DspxProjectContext>(this);
        d->metronomeControlMixer = new talcs::MixerAudioSource;
        d->projectContext->preMixer()->prependSource(d->metronomeControlMixer, true);
    }

    ProjectAudioContext::~ProjectAudioContext() = default;

    ProjectAudioContext *ProjectAudioContext::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<ProjectAudioContext>();
    }

    Core::ProjectWindowInterface *ProjectAudioContext::windowHandle() const {
        Q_D(const ProjectAudioContext);
        return d->windowHandle;
    }

    ProjectAudioContext::PlaybackStatus ProjectAudioContext::status() const {
        Q_D(const ProjectAudioContext);
        return d->status;
    }

    void ProjectAudioContext::setStatus(PlaybackStatus status) {
        Q_D(ProjectAudioContext);
        switch (status) {
        case Stopped:
        case Playing:
        case Paused:
        case Recording:
        case RecordingPaused:
            break;
        default:
            qWarning() << "The requested project playback status is invalid." << static_cast<int>(status);
            return;
        }
        if (d->playbackStatusChangeHandler && d->playbackStatusChangeHandler(status))
            return;
        if (d->status == status)
            return;
        d->status = status;
        Q_EMIT statusChanged(status);
        Q_EMIT transportStateChanged();
    }

    bool ProjectAudioContext::isRunning() const {
        const auto currentStatus = status();
        return currentStatus == Playing || currentStatus == Recording;
    }

    bool ProjectAudioContext::isRecording() const {
        const auto currentStatus = status();
        return currentStatus == Recording || currentStatus == RecordingPaused;
    }

    bool ProjectAudioContext::isPaused() const {
        const auto currentStatus = status();
        return currentStatus == Paused || currentStatus == RecordingPaused;
    }

    dspx::SingingClip *ProjectAudioContext::playbackSuppressedClip() const {
        Q_D(const ProjectAudioContext);
        return d->playbackSuppressedClip;
    }

    void ProjectAudioContext::setPlaybackSuppressedClip(dspx::SingingClip *clip) {
        Q_D(ProjectAudioContext);
        if (d->playbackSuppressedClip == clip)
            return;
        disconnect(d->suppressedClipDestroyedConnection);
        d->playbackSuppressedClip = clip;
        if (clip) {
            d->suppressedClipDestroyedConnection = connect(clip, &QObject::destroyed, this, [this] {
                Q_D(ProjectAudioContext);
                d->playbackSuppressedClip.clear();
                Q_EMIT playbackSuppressedClipChanged(nullptr);
            });
        }
        Q_EMIT playbackSuppressedClipChanged(clip);
    }

    talcs::MixerAudioSource *ProjectAudioContext::preMixer() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->preMixer();
    }

    talcs::MixerAudioSource *ProjectAudioContext::metronomeControlMixer() const {
        Q_D(const ProjectAudioContext);
        return d->metronomeControlMixer;
    }

    talcs::TransportAudioSource *ProjectAudioContext::transport() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->transport();
    }

    talcs::PositionableMixerAudioSource *ProjectAudioContext::postMixer() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->postMixer();
    }

    talcs::PositionableMixerAudioSource *ProjectAudioContext::masterControlMixer() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->masterControlMixer();
    }

    talcs::PositionableMixerAudioSource *ProjectAudioContext::masterTrackMixer() const {
        Q_D(const ProjectAudioContext);
        return d->projectContext->masterTrackMixer();
    }

    ProjectAudioContext *ProjectAudioContextPrivate::create(Core::ProjectWindowInterface *windowHandle) {
        return new ProjectAudioContext(windowHandle);
    }

    ProjectAudioContextPrivate *ProjectAudioContextPrivate::of(ProjectAudioContext *q) {
        return q->d_func();
    }

}

#include "moc_ProjectAudioContext.cpp"
