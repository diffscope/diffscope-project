#ifndef DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_P_H
#define DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_P_H

#include <audio/AudioClipAudioContext.h>

namespace talcs {
    class DspxAudioClipContext;
    class DspxTrackContext;
}

namespace Audio {

    class AudioClipAudioContextPrivate {
        Q_DECLARE_PUBLIC(AudioClipAudioContext)
    public:
        AudioClipAudioContext *q_ptr{};
        Core::ProjectWindowInterface *windowHandle{};
        dspx::AudioClip *clip{};
        talcs::DspxTrackContext *trackContext{};
        talcs::DspxAudioClipContext *clipContext{};

        static AudioClipAudioContext *create(Core::ProjectWindowInterface *windowHandle, dspx::AudioClip *clip, talcs::DspxTrackContext *trackContext);
        static AudioClipAudioContextPrivate *of(AudioClipAudioContext *q);
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_P_H
