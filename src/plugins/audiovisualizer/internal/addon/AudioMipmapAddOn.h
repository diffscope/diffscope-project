#ifndef DIFFSCOPE_AUDIO_VISUALIZER_AUDIOMIPMAPADDON_H
#define DIFFSCOPE_AUDIO_VISUALIZER_AUDIOMIPMAPADDON_H

#include <CoreApi/windowinterface.h>

#include <SVSCraftCore/WaveformMipmap.h>

#include <QHash>
#include <QSet>

namespace dspx {
    class AudioClip;
    class Clip;
    class Track;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace AudioVisualizer::Internal {

    class AudioThumbnailWaveformThumbnail;

    class AudioMipmapAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit AudioMipmapAddOn(QObject *parent = nullptr);
        ~AudioMipmapAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static AudioMipmapAddOn *of(Core::ProjectWindowInterface *windowHandle);

        void mapThumbnailView(dspx::AudioClip *clip, AudioThumbnailWaveformThumbnail *view);
        void unmapThumbnailVIew(dspx::AudioClip *clip, AudioThumbnailWaveformThumbnail *view);

    private Q_SLOTS:
        void addClip(dspx::Clip *clip);
        void removeClip(dspx::Clip *clip);
        void reloadAudioClip(dspx::AudioClip *clip);
        void handleMipmapLoaded(dspx::AudioClip *clip, double sampleRate, const SVS::WaveformMipmap &mipmap);

    Q_SIGNALS:
        void mipmapLoaded(dspx::AudioClip *clip, double sampleRate, const SVS::WaveformMipmap &mipmap);

    private:
        void loadAudioClip(dspx::AudioClip *clip);
        void processAudioClipMipmap(dspx::AudioClip *clip, double sampleRate, const SVS::WaveformMipmap &mipmap);

        QHash<dspx::AudioClip *, double> m_audioClipSampleRates;
        QHash<dspx::AudioClip *, SVS::WaveformMipmap> m_audioClipMipmaps;
        QHash<dspx::AudioClip *, QSet<AudioThumbnailWaveformThumbnail *>> m_audioClipThumbnailViews;
    };

}

Q_DECLARE_METATYPE(SVS::WaveformMipmap)

#endif // DIFFSCOPE_AUDIO_VISUALIZER_AUDIOMIPMAPADDON_H
