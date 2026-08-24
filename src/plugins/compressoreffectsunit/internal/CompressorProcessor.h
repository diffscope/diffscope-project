// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSORPROCESSOR_H
#define DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSORPROCESSOR_H

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <TalcsCore/AudioSource.h>

namespace CompressorEffectsUnit::Internal {

    class CompressorProcessor : public talcs::AudioSource {
    public:
        struct MeterValues {
            float inputLevelDb{};
            std::array<float, 2> outputLevelDb{};
            std::array<float, 2> gainReductionDb{};
        };

        CompressorProcessor();
        ~CompressorProcessor() override;

        void setParameters(double thresholdDb, double ratio,
                           double attackMilliseconds, double releaseMilliseconds);
        bool takeMeterValues(MeterValues &values);
        void discardMeterValues();

        bool open(qint64 bufferSize, double sampleRate) override;
        void close() override;

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override;

    private:
        static constexpr std::size_t meterQueueCapacity = 256;

        static float coefficient(double sampleRate, float timeMilliseconds);
        static float decibelsFromGain(float gain);
        void publishMeterValues(const MeterValues &values);

        std::atomic<float> m_thresholdDb;
        std::atomic<float> m_ratio;
        std::atomic<float> m_attackMilliseconds;
        std::atomic<float> m_releaseMilliseconds;
        std::array<MeterValues, meterQueueCapacity> m_meterQueue;
        std::atomic<std::uint64_t> m_meterWriteIndex{};
        std::atomic<std::uint64_t> m_meterReadIndex{};
        double m_sampleRate{44100.0};
        float m_envelope{};
    };

}

#endif // DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSORPROCESSOR_H
