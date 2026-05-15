#ifndef DIFFSCOPE_AUDIO_VISUALIZER_AUDIOTHUMBNAILWAVEFORMTHUMBNAIL_H
#define DIFFSCOPE_AUDIO_VISUALIZER_AUDIOTHUMBNAILWAVEFORMTHUMBNAIL_H

#include <SVSCraftQuick/WaveformThumbnail.h>

#include <QList>
#include <QMetaObject>
#include <QPointer>

#include <qqmlintegration.h>

namespace dspx {
    class AudioClip;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace AudioVisualizer::Internal {

    class AudioMipmapAddOn;

    class AudioThumbnailWaveformThumbnail : public SVS::WaveformThumbnail {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(dspx::AudioClip *audioClip READ audioClip WRITE setAudioClip NOTIFY audioClipChanged)
        Q_PROPERTY(Core::ProjectWindowInterface *projectWindowInterface READ projectWindowInterface WRITE setProjectWindowInterface NOTIFY projectWindowInterfaceChanged)
        Q_PROPERTY(double viewportOffset READ viewportOffset WRITE setViewportOffset NOTIFY viewportOffsetChanged)
        Q_PROPERTY(double viewportLength READ viewportLength WRITE setViewportLength NOTIFY viewportLengthChanged)
        Q_PROPERTY(double sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged)

    public:
        explicit AudioThumbnailWaveformThumbnail(QQuickItem *parent = nullptr);
        ~AudioThumbnailWaveformThumbnail() override;

        dspx::AudioClip *audioClip() const;
        void setAudioClip(dspx::AudioClip *audioClip);

        Core::ProjectWindowInterface *projectWindowInterface() const;
        void setProjectWindowInterface(Core::ProjectWindowInterface *projectWindowInterface);

        double viewportOffset() const;
        void setViewportOffset(double viewportOffset);

        double viewportLength() const;
        void setViewportLength(double viewportLength);

        double sampleRate() const;
        void setSampleRate(double sampleRate);

    Q_SIGNALS:
        void audioClipChanged();
        void projectWindowInterfaceChanged();
        void viewportOffsetChanged();
        void viewportLengthChanged();
        void sampleRateChanged();

    private:
        void resetMapping();
        void updateMapping();
        void reconnectAudioClip();
        void reconnectProjectWindowInterface();
        void updateWaveformGeometry();
        void clearWaveformGeometry();
        double tickToMillisecond(double tick) const;
        double tickRangeToSamples(double startTick, double endTick) const;

        QPointer<dspx::AudioClip> m_audioClip;
        QPointer<Core::ProjectWindowInterface> m_projectWindowInterface;
        QPointer<AudioMipmapAddOn> m_mipmapAddOn;
        QList<QMetaObject::Connection> m_audioClipConnections;
        QList<QMetaObject::Connection> m_projectWindowInterfaceConnections;
        double m_viewportOffset = 0.0;
        double m_viewportLength = 0.0;
        double m_sampleRate = 0.0;
    };

}

#endif // DIFFSCOPE_AUDIO_VISUALIZER_AUDIOTHUMBNAILWAVEFORMTHUMBNAIL_H
