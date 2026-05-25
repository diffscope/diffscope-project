#ifndef DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_H
#define DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_H

#include <QObject>

#include <audio/audioglobal.h>

namespace talcs {
    class AbstractAudioFormatIO;
}

namespace Audio {

    class PreviewSoundPlayerPrivate;

    class AUDIO_EXPORT PreviewSoundPlayer : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(PreviewSoundPlayer)

    public:
        explicit PreviewSoundPlayer(talcs::AbstractAudioFormatIO *audioFormatIo, QObject *parent = nullptr);
        ~PreviewSoundPlayer() override;

        void play();
        void stop();

    Q_SIGNALS:
        void finished();

    private:
        QScopedPointer<PreviewSoundPlayerPrivate> d_ptr;
    };

}

using PreviewSoundPlayer = Audio::PreviewSoundPlayer;

#endif // DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_H
