#include "TrackAudioContext.h"
#include "TrackAudioContext_p.h"

#include <QHash>

#include <TalcsDspx/DspxProjectContext.h>
#include <TalcsDspx/DspxTrackContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <dspxmodelORM/Track.h>

namespace Audio {

    static QHash<dspx::Track *, TrackAudioContext *> s_trackAudioContexts;

    TrackAudioContext::TrackAudioContext(Core::ProjectWindowInterface *windowHandle, dspx::Track *track, talcs::DspxProjectContext *projectContext, int index)
        : QObject(windowHandle), d_ptr(new TrackAudioContextPrivate) {
        Q_D(TrackAudioContext);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
        d->track = track;
        d->projectContext = projectContext;
        d->trackContext = projectContext->addTrack(index);
        s_trackAudioContexts.insert(track, this);
    }

    TrackAudioContext::~TrackAudioContext() {
        Q_D(TrackAudioContext);
        s_trackAudioContexts.remove(d->track);
        if (d->projectContext) {
            const auto index = d->projectContext->tracks().indexOf(d->trackContext);
            if (index >= 0) {
                d->projectContext->removeTrack(index);
            }
        }
    }

    TrackAudioContext *TrackAudioContext::of(dspx::Track *track) {
        return s_trackAudioContexts.value(track);
    }

    Core::ProjectWindowInterface *TrackAudioContext::windowHandle() const {
        Q_D(const TrackAudioContext);
        return d->windowHandle;
    }

    dspx::Track *TrackAudioContext::track() const {
        Q_D(const TrackAudioContext);
        return d->track;
    }

    talcs::PositionableMixerAudioSource *TrackAudioContext::controlMixer() const {
        Q_D(const TrackAudioContext);
        return d->trackContext->controlMixer();
    }

    talcs::PositionableMixerAudioSource *TrackAudioContext::trackMixer() const {
        Q_D(const TrackAudioContext);
        return d->trackContext->trackMixer();
    }

    talcs::AudioSourceClipSeries *TrackAudioContext::clipSeries() const {
        Q_D(const TrackAudioContext);
        return d->trackContext->clipSeries();
    }

    TrackAudioContext *TrackAudioContextPrivate::create(Core::ProjectWindowInterface *windowHandle, dspx::Track *track, talcs::DspxProjectContext *projectContext, int index) {
        return new TrackAudioContext(windowHandle, track, projectContext, index);
    }

    TrackAudioContextPrivate *TrackAudioContextPrivate::of(TrackAudioContext *q) {
        return q->d_func();
    }

}

#include "moc_TrackAudioContext.cpp"
