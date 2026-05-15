#ifndef DIFFSCOPE_AUDIO_TRACKAUDIOCONTEXT_H
#define DIFFSCOPE_AUDIO_TRACKAUDIOCONTEXT_H

#include <QObject>

#include <audio/audioglobal.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class Track;
}

namespace talcs {
    class AudioSourceClipSeries;
    class DspxProjectContext;
    class PositionableMixerAudioSource;
}

namespace Audio {

    namespace Internal {
        class ProjectAudioAddOn;
    }

    class TrackAudioContextPrivate;

    class AUDIO_EXPORT TrackAudioContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(TrackAudioContext)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(dspx::Track *track READ track CONSTANT)

    public:
        ~TrackAudioContext() override;

        static TrackAudioContext *of(dspx::Track *track);

        Core::ProjectWindowInterface *windowHandle() const;
        dspx::Track *track() const;

        talcs::PositionableMixerAudioSource *controlMixer() const;
        talcs::PositionableMixerAudioSource *trackMixer() const;
        talcs::AudioSourceClipSeries *clipSeries() const;

    private:
        friend class TrackAudioContextPrivate;
        friend class Internal::ProjectAudioAddOn;
        explicit TrackAudioContext(Core::ProjectWindowInterface *windowHandle, dspx::Track *track, talcs::DspxProjectContext *projectContext, int index);

        QScopedPointer<TrackAudioContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_TRACKAUDIOCONTEXT_H
