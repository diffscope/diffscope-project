#ifndef DIFFSCOPE_AUDIO_PROJECTAUDIOADDON_H
#define DIFFSCOPE_AUDIO_PROJECTAUDIOADDON_H

#include <CoreApi/windowinterface.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class Track;
}

namespace Audio {
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

    private Q_SLOTS:
        void addTrack(int index, dspx::Track *track);
        void removeTrack(int index, dspx::Track *track);
        void rotateTrack(int leftIndex, int middleIndex, int rightIndex);

    private:
        void syncMasterControl();
        void syncTrackControl(dspx::Track *track, TrackAudioContext *context);

        ProjectAudioContext *m_context{};
    };

}

#endif // DIFFSCOPE_AUDIO_PROJECTAUDIOADDON_H
