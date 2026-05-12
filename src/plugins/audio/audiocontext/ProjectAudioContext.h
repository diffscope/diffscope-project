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

    public:
        ~ProjectAudioContext() override;

        static ProjectAudioContext *of(Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;

        talcs::MixerAudioSource *preMixer() const;
        talcs::TransportAudioSource *transport() const;
        talcs::PositionableMixerAudioSource *postMixer() const;
        talcs::PositionableMixerAudioSource *masterControlMixer() const;
        talcs::PositionableMixerAudioSource *masterTrackMixer() const;

    private:
        friend class ProjectAudioContextPrivate;
        friend class Internal::ProjectAudioAddOn;
        explicit ProjectAudioContext(Core::ProjectWindowInterface *windowHandle);

        QScopedPointer<ProjectAudioContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_H
