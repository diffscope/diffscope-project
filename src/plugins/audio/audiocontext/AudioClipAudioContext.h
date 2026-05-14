#ifndef DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_H
#define DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_H

#include <QObject>
#include <qqmlintegration.h>

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
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AudioClipAudioContext)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(dspx::AudioClip *clip READ clip CONSTANT)
        Q_PROPERTY(Status status READ status NOTIFY statusChanged)
        Q_PROPERTY(QString realAudioPath READ realAudioPath NOTIFY realAudioPathChanged)

    public:
        enum Status {
            Unknown,
            Ready,
            FileNotFound,
            FileLoadFailed,
            FileMoved,
            FileContentChanged,
        };
        Q_ENUM(Status)

        ~AudioClipAudioContext() override;

        static AudioClipAudioContext *of(dspx::AudioClip *clip);

        Core::ProjectWindowInterface *windowHandle() const;
        dspx::AudioClip *clip() const;

        Status status() const;
        QString realAudioPath() const;

        talcs::PositionableMixerAudioSource *controlMixer() const;
        talcs::PositionableMixerAudioSource *clipMixer() const;
        talcs::PositionableAudioSource *contentSource() const;

    Q_SIGNALS:
        void statusChanged(Status status);
        void realAudioPathChanged(const QString &realAudioPath);

    private:
        friend class AudioClipAudioContextPrivate;
        friend class Internal::ProjectAudioAddOn;
        explicit AudioClipAudioContext(Core::ProjectWindowInterface *windowHandle, dspx::AudioClip *clip, talcs::DspxTrackContext *trackContext);

        QScopedPointer<AudioClipAudioContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_H
