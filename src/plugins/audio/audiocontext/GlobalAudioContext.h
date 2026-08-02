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
        Q_PROPERTY(bool metronomeEnabled READ metronomeEnabled WRITE setMetronomeEnabled NOTIFY metronomeEnabledChanged)
        Q_PROPERTY(double metronomeGain READ metronomeGain WRITE setMetronomeGain NOTIFY metronomeGainChanged)
        Q_PROPERTY(double metronomePan READ metronomePan WRITE setMetronomePan NOTIFY metronomePanChanged)
        Q_PROPERTY(double deviceGain READ deviceGain WRITE setDeviceGain NOTIFY deviceGainChanged)
        Q_PROPERTY(double devicePan READ devicePan WRITE setDevicePan NOTIFY devicePanChanged)

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
        static bool metronomeEnabled();
        static void setMetronomeEnabled(bool enabled);
        static double metronomeGain();
        static void setMetronomeGain(double gain);
        static double metronomePan();
        static void setMetronomePan(double pan);
        static double deviceGain();
        static void setDeviceGain(double gain);
        static double devicePan();
        static void setDevicePan(double pan);

        static talcs::FormatManager *formatManager();

    Q_SIGNALS:
        void deviceChanged();
        void bufferSizeChanged(qint64 bufferSize);
        void sampleRateChanged(double sampleRate);
        void metronomeEnabledChanged(bool enabled);
        void metronomeGainChanged(double gain);
        void metronomePanChanged(double pan);
        void deviceGainChanged(double gain);
        void devicePanChanged(double pan);

    private:
        explicit GlobalAudioContext(QObject *parent = nullptr);

        QScopedPointer<GlobalAudioContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_H
