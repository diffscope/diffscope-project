#ifndef DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H
#define DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H

#include <QObject>

namespace Core {
    class ProjectWindowInterface;
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

    class ProjectAudioContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ProjectAudioContext)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(PlaybackStatus status READ status WRITE setStatus NOTIFY statusChanged)

    public:
        ~ProjectAudioContext() override;

        static ProjectAudioContext *of(Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;

        enum PlaybackStatus {
            Stopped,
            Playing,
            Paused,
        };
        Q_ENUM(PlaybackStatus)
        PlaybackStatus status() const;
        void setStatus(PlaybackStatus status);

        talcs::MixerAudioSource *preMixer() const;
        talcs::TransportAudioSource *transport() const;
        talcs::PositionableMixerAudioSource *postMixer() const;
        talcs::PositionableMixerAudioSource *masterControlMixer() const;
        talcs::PositionableMixerAudioSource *masterTrackMixer() const;

    Q_SIGNALS:
        void statusChanged(PlaybackStatus status);

    private:
        friend class ProjectAudioContextPrivate;
        friend class Internal::ProjectAudioAddOn;
        explicit ProjectAudioContext(Core::ProjectWindowInterface *windowHandle);

        QScopedPointer<ProjectAudioContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H
