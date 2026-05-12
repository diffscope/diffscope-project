#include "AudioClipAudioContext.h"
#include "AudioClipAudioContext_p.h"

#include <QHash>

#include <TalcsCore/BufferingAudioSource.h>
#include <TalcsCore/PositionableAudioSource.h>
#include <TalcsCore/PositionableMixerAudioSource.h>
#include <TalcsDspx/DspxAudioClipContext.h>
#include <TalcsDspx/DspxTrackContext.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <dspxmodel/AudioClip.h>

namespace Audio {

    static QHash<dspx::AudioClip *, AudioClipAudioContext *> s_audioClipAudioContexts;

    static quintptr audioClipId(dspx::AudioClip *clip) {
        return reinterpret_cast<quintptr>(clip);
    }

    AudioClipAudioContext::AudioClipAudioContext(Core::ProjectWindowInterface *windowHandle, dspx::AudioClip *clip, talcs::DspxTrackContext *trackContext)
        : QObject(windowHandle), d_ptr(new AudioClipAudioContextPrivate) {
        Q_D(AudioClipAudioContext);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
        d->clip = clip;
        d->trackContext = trackContext;
        d->clipContext = trackContext->addAudioClip(audioClipId(clip));
        s_audioClipAudioContexts.insert(clip, this);
    }

    AudioClipAudioContext::~AudioClipAudioContext() {
        Q_D(AudioClipAudioContext);
        s_audioClipAudioContexts.remove(d->clip);
        d->trackContext->removeAudioClip(audioClipId(d->clip));
    }

    AudioClipAudioContext *AudioClipAudioContext::of(dspx::AudioClip *clip) {
        return s_audioClipAudioContexts.value(clip);
    }

    Core::ProjectWindowInterface *AudioClipAudioContext::windowHandle() const {
        Q_D(const AudioClipAudioContext);
        return d->windowHandle;
    }

    dspx::AudioClip *AudioClipAudioContext::clip() const {
        Q_D(const AudioClipAudioContext);
        return d->clip;
    }

    talcs::PositionableMixerAudioSource *AudioClipAudioContext::controlMixer() const {
        Q_D(const AudioClipAudioContext);
        return d->clipContext->controlMixer();
    }

    talcs::PositionableMixerAudioSource *AudioClipAudioContext::clipMixer() const {
        Q_D(const AudioClipAudioContext);
        return d->clipContext->clipMixer();
    }

    talcs::PositionableAudioSource *AudioClipAudioContext::contentSource() const {
        Q_D(const AudioClipAudioContext);
        return d->clipContext->contentSource();
    }

    AudioClipAudioContext *AudioClipAudioContextPrivate::create(Core::ProjectWindowInterface *windowHandle, dspx::AudioClip *clip, talcs::DspxTrackContext *trackContext) {
        return new AudioClipAudioContext(windowHandle, clip, trackContext);
    }

    AudioClipAudioContextPrivate *AudioClipAudioContextPrivate::of(AudioClipAudioContext *q) {
        return q->d_func();
    }

}

#include "moc_AudioClipAudioContext.cpp"
