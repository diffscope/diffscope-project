// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectAudioContext.h"
#include "ProjectAudioContext_p.h"

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsDspx/DspxProjectContext.h>

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
        if (d->status == status) {
            return;
        }
        d->status = status;
        Q_EMIT statusChanged(status);
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
