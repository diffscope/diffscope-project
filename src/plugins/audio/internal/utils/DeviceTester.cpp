#include "DeviceTester.h"

#include <QtMath>

namespace Audio::Internal {

    DeviceTester::DeviceTester() = default;

    DeviceTester::~DeviceTester() = default;

    bool DeviceTester::open(qint64 bufferSize, double sampleRate) {
        constexpr static double PI = M_PI;
        m_sound.resize(1, qint64(sampleRate));
        double fadeIn = 0.005;
        double fadeOut = 0.005;
        double rate = std::pow(0.99, 20000.0 / sampleRate);
        qint64 i, j;
        for (i = 0; i < m_sound.sampleCount() && fadeIn < 1.0; i++) {
            m_sound.setSample(0, i, float(0.25 * std::sin(2.0 * PI * 440.0 / sampleRate * double(i)) * fadeIn));
            fadeIn /= rate;
        }
        for (j = m_sound.sampleCount() - 1; j >= 0 && fadeOut < 1.0; j--) {
            m_sound.setSample(0, j, float(0.25 * std::sin(2.0 * PI * 440.0 / sampleRate * double(j)) * fadeOut));
            fadeOut /= rate;
        }
        for (;i <= j; i++) {
            m_sound.setSample(0, i, float(0.25 * std::sin(2.0 * PI * 440.0 / sampleRate * double(i))));
        }
        m_pos = -1;
        return AudioSource::open(bufferSize, sampleRate);
    }

    void DeviceTester::close() {
        AudioSource::close();
    }

    void DeviceTester::playTestSound() {
        m_pos = 0;
    }

    qint64 DeviceTester::processReading(const talcs::AudioSourceReadData &readData) {
        qint64 pos = m_pos;
        if (pos < 0 || pos >= m_sound.sampleCount())
            return readData.length;
        qint64 length = qMin(readData.length, m_sound.sampleCount() - pos);
        for (int ch = 0; ch < readData.buffer->channelCount(); ch++) {
            readData.buffer->setSampleRange(ch, readData.startPos, length, m_sound, 0, pos);
            readData.buffer->clear(ch, readData.startPos + length, readData.length - length);
        }
        m_pos += length;
        return readData.length;
    }

}
