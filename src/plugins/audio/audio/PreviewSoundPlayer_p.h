#ifndef DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_P_H
#define DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_P_H

#include <audio/PreviewSoundPlayer.h>

#include <memory>

namespace talcs {
    class AudioFormatInputSource;
}

namespace Audio {

    class PreviewSoundFinishedFilter;

    class PreviewSoundPlayerPrivate {
        Q_DECLARE_PUBLIC(PreviewSoundPlayer)
    public:
        explicit PreviewSoundPlayerPrivate(talcs::AbstractAudioFormatIO *audioFormatIo);
        ~PreviewSoundPlayerPrivate();

        void destroySource(talcs::AudioFormatInputSource *expectedSource = nullptr);
        void finish(talcs::AudioFormatInputSource *source, quint64 serial);

        PreviewSoundPlayer *q_ptr{};
        std::unique_ptr<talcs::AbstractAudioFormatIO> audioFormatIo;
        std::unique_ptr<PreviewSoundFinishedFilter> finishedFilter;
        std::unique_ptr<talcs::AudioFormatInputSource> source;
        quint64 serial{};
    };

}

#endif // DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_P_H
