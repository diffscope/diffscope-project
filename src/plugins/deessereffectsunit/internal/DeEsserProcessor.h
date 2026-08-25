// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSERPROCESSOR_H
#define DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSERPROCESSOR_H

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <TalcsCore/AudioSource.h>

#include <deessereffectsunit/internal/DeEsserParameters.h>

namespace DeEsserEffectsUnit::Internal {

    class DeEsserProcessor : public talcs::AudioSource {
    public:
        static constexpr int spectrumBinCount = deEsserSpectrumBinCount;

        struct MeterValues {
            std::array<float, 2> bandLevelDb{};
            std::array<float, 2> gainReductionDb{};
        };

        struct SpectrumFrame {
            std::array<float, spectrumBinCount> levelsDb{};
        };

        DeEsserProcessor();
        ~DeEsserProcessor() override;

        void setParameters(double frequencyHz, double bandwidthHz,
                           double thresholdDb, bool outputSibilanceOnly);
        void refresh();
        void setAnalysisEnabled(bool enabled);
        bool takeMeterValues(MeterValues &values);
        void discardMeterValues();
        bool takeSpectrumFrame(SpectrumFrame &frame);
        void discardSpectrumFrames();
        double currentSampleRate() const;

        bool open(qint64 bufferSize, double sampleRate) override;
        void close() override;

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override;

    private:
        static constexpr std::size_t meterQueueCapacity = 256;
        static constexpr std::size_t spectrumQueueCapacity = 8;

        struct ParameterSnapshot {
            float frequencyHz{};
            float bandwidthHz{};
            float thresholdDb{};
            bool outputSibilanceOnly{};
        };

        class ProcessingEngine;

        bool tryReadParameterSnapshot(ParameterSnapshot &snapshot,
                                      std::uint64_t &revision) const;
        void publishMeterValues(const MeterValues &values);
        void publishSpectrumFrame(const SpectrumFrame &frame);

        std::atomic<std::uint32_t> m_frequencyBits{};
        std::atomic<std::uint32_t> m_bandwidthBits{};
        std::atomic<std::uint32_t> m_thresholdBits{};
        std::atomic<bool> m_outputSibilanceOnly{};
        std::atomic<std::uint64_t> m_parameterRevision{};
        std::atomic<std::uint64_t> m_sampleRateBits{};
        std::atomic<bool> m_analysisEnabled{};
        std::atomic<bool> m_refreshRequested{};
        std::array<MeterValues, meterQueueCapacity> m_meterQueue;
        std::atomic<std::uint64_t> m_meterWriteIndex{};
        std::atomic<std::uint64_t> m_meterReadIndex{};
        std::array<SpectrumFrame, spectrumQueueCapacity> m_spectrumQueue;
        std::atomic<std::uint64_t> m_spectrumWriteIndex{};
        std::atomic<std::uint64_t> m_spectrumReadIndex{};
        std::unique_ptr<ProcessingEngine> m_engine;
        std::uint64_t m_appliedRevision{};
    };

}

#endif // DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSERPROCESSOR_H
