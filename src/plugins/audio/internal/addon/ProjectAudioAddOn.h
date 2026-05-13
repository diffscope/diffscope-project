#ifndef DIFFSCOPE_AUDIO_PROJECTAUDIOADDON_H
#define DIFFSCOPE_AUDIO_PROJECTAUDIOADDON_H

#include <CoreApi/windowinterface.h>

#include <QHash>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class AudioClip;
    class Clip;
    class Track;
}

namespace talcs {
    class AbstractAudioFormatIO;
}

namespace Audio {
    class AudioClipAudioContext;
    class ProjectAudioContext;
    class TrackAudioContext;
}

namespace Audio::Internal {

    class ProjectAudioAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit ProjectAudioAddOn(QObject *parent = nullptr);
        ~ProjectAudioAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static ProjectAudioAddOn *of(Core::ProjectWindowInterface *windowHandle);

        void addAudioClipCache(dspx::AudioClip *clip, talcs::AbstractAudioFormatIO *io);
        talcs::AbstractAudioFormatIO *takeAudioClipCache(dspx::AudioClip *clip);

    private Q_SLOTS:
        void addTrack(int index, dspx::Track *track);
        void removeTrack(int index, dspx::Track *track);
        void rotateTrack(int leftIndex, int middleIndex, int rightIndex);

    private:
        void syncMasterControl();
        void syncTrackControl(dspx::Track *track, TrackAudioContext *context);
        void syncTrackClips(dspx::Track *track, TrackAudioContext *context);
        void addClip(dspx::Clip *clip);
        void removeClip(dspx::Clip *clip);
        void syncAudioClip(dspx::AudioClip *clip, AudioClipAudioContext *context);
        void loadAudioClip(dspx::AudioClip *clip, AudioClipAudioContext *context);
        void reloadAudioClip(dspx::AudioClip *clip, AudioClipAudioContext *context);
        void notifyAudioClipStatus(dspx::AudioClip *clip, AudioClipAudioContext *context);

        ProjectAudioContext *m_context{};
        QHash<dspx::AudioClip *, talcs::AbstractAudioFormatIO *> m_audioClipCache;
    };

}

#endif // DIFFSCOPE_AUDIO_PROJECTAUDIOADDON_H
