#ifndef DIFFSCOPE_AUDIO_WAVEFORMSINGERADDON_H
#define DIFFSCOPE_AUDIO_WAVEFORMSINGERADDON_H

#include <cstdint>
#include <memory>

#include <QHash>

#include <CoreApi/windowinterface.h>

#include <audio/internal/WaveformSingerModel.h>

namespace dspx {
    class Clip;
    class SingingClip;
    class Track;
}

namespace Audio {
    class ProjectAudioContext;
}

namespace Audio::Internal {

    class WaveformSingerAddOn final : public Core::WindowInterfaceAddOn {
        Q_OBJECT

    public:
        explicit WaveformSingerAddOn(QObject *parent = nullptr);
        ~WaveformSingerAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        class TrackBinding;
        class ClipBinding;

        void addTrack(int index, dspx::Track *track);
        void removeTrack(int index, dspx::Track *track);
        void addClip(dspx::Clip *clip, TrackBinding *trackBinding);
        void removeClip(dspx::Clip *clip);
        void syncTempo();
        void refreshRanges();
        double activeSampleRate() const;

        Audio::ProjectAudioContext *m_audioContext{};
        std::shared_ptr<WaveformSingerTempoModel> m_tempoModel;
        QHash<dspx::Track *, TrackBinding *> m_tracks;
        QHash<dspx::SingingClip *, ClipBinding *> m_clips;
        std::uint64_t m_revision{};
    };

}

#endif // DIFFSCOPE_AUDIO_WAVEFORMSINGERADDON_H
