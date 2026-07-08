#ifndef DIFFSCOPE_AUDIO_PLAYBACKADDON_H
#define DIFFSCOPE_AUDIO_PLAYBACKADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

#include <audio/ProjectAudioContext.h>

namespace Core {
    class ProjectTimeline;
    class ProjectWindowInterface;
}

namespace dspx {
    class Model;
}

namespace Audio::Internal {

    class PlaybackAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(ProjectAudioContext *projectAudioContext MEMBER m_context CONSTANT)
    public:
        explicit PlaybackAddOn(QObject *parent = nullptr);
        ~PlaybackAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static PlaybackAddOn *of(Core::ProjectWindowInterface *windowHandle);

        Q_INVOKABLE void play();
        Q_INVOKABLE void pause();
        Q_INVOKABLE void stop();
        Q_INVOKABLE void togglePlayback();
        Q_INVOKABLE QObject *getPlayAction() const;
        Q_INVOKABLE QObject *getPauseAction() const;

    private:
        qint64 tickToSample(int tick) const;
        int sampleToTick(qint64 sample) const;

        void syncLoopingRange();
        void handlePlaybackStatusChanged(Audio::ProjectAudioContext::PlaybackStatus status);
        void handlePlaybackPositionChanged(int positionTick);

        ProjectAudioContext *m_context{};
        Core::ProjectTimeline *m_projectTimeline{};
        dspx::Model *m_documentModel{};
        bool m_transportPositionFlag{true};
        Audio::ProjectAudioContext::PlaybackStatus m_lastStatus{};
    };

}

#endif // DIFFSCOPE_AUDIO_PLAYBACKADDON_H
