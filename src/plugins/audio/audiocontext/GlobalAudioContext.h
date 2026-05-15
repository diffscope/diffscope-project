#ifndef DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_H
#define DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_H

#include <QObject>

#include <audio/audioglobal.h>

namespace talcs {
    class AudioDevice;
    class AudioDriver;
    class AudioDriverManager;
    class AudioSourcePlayback;
    class FormatManager;
    class MixerAudioSource;
}

namespace Audio {

    class GlobalAudioContextPrivate;

    class AUDIO_EXPORT GlobalAudioContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(GlobalAudioContext)
        Q_PROPERTY(qint64 bufferSize READ bufferSize NOTIFY bufferSizeChanged)
        Q_PROPERTY(double sampleRate READ sampleRate NOTIFY sampleRateChanged)

    public:
        ~GlobalAudioContext() override;

        static GlobalAudioContext *instance();

        static talcs::AudioDriverManager *driverManager();
        static talcs::AudioDriver *driver();
        static talcs::AudioDevice *device();
        static talcs::AudioSourcePlayback *playback();
        static talcs::MixerAudioSource *controlMixer();
        static talcs::MixerAudioSource *preMixer();
        static qint64 bufferSize();
        static double sampleRate();

        static talcs::FormatManager *formatManager();

    Q_SIGNALS:
        void deviceChanged();
        void bufferSizeChanged(qint64 bufferSize);
        void sampleRateChanged(double sampleRate);

    private:
        explicit GlobalAudioContext(QObject *parent = nullptr);

        QScopedPointer<GlobalAudioContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_H
