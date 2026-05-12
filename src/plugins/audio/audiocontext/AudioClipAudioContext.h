#ifndef DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_H
#define DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_H

#include <QObject>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class AudioClip;
}

namespace talcs {
    class DspxTrackContext;
    class PositionableAudioSource;
    class PositionableMixerAudioSource;
}

namespace Audio {

    namespace Internal {
        class ProjectAudioAddOn;
    }

    class AudioClipAudioContextPrivate;

    class AudioClipAudioContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(AudioClipAudioContext)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(dspx::AudioClip *clip READ clip CONSTANT)

    public:
        ~AudioClipAudioContext() override;

        static AudioClipAudioContext *of(dspx::AudioClip *clip);

        Core::ProjectWindowInterface *windowHandle() const;
        dspx::AudioClip *clip() const;

        talcs::PositionableMixerAudioSource *controlMixer() const;
        talcs::PositionableMixerAudioSource *clipMixer() const;
        talcs::PositionableAudioSource *contentSource() const;

    private:
        friend class AudioClipAudioContextPrivate;
        friend class Internal::ProjectAudioAddOn;
        explicit AudioClipAudioContext(Core::ProjectWindowInterface *windowHandle, dspx::AudioClip *clip, talcs::DspxTrackContext *trackContext);

        QScopedPointer<AudioClipAudioContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_H
