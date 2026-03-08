#ifndef DIFFSCOPE_AUDIO_DEVICETESTER_H
#define DIFFSCOPE_AUDIO_DEVICETESTER_H

#include <limits>

#include <QObject>

#include <TalcsCore/AudioSource.h>
#include <TalcsCore/AudioBuffer.h>

namespace Audio::Internal {

    class DeviceTester : public talcs::AudioSource {
    public:
        explicit DeviceTester();
        ~DeviceTester() override;

        bool open(qint64 bufferSize, double sampleRate) override;
        void close() override;

        void playTestSound();

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override;

    private:
        talcs::AudioBuffer m_sound;
        QAtomicInteger<qint64> m_pos = -1;
    };

}

#endif // DIFFSCOPE_AUDIO_DEVICETESTER_H