// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERPROCESSOR_H
#define DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERPROCESSOR_H

#include <array>
#include <atomic>
#include <complex>
#include <cstddef>
#include <cstdint>

#include <TalcsCore/AudioSource.h>

#include <equalizereffectsunit/internal/EqualizerParameters.h>

#include <signalsmith-dsp/filters.h>
#include <signalsmith-dsp/spectral.h>

namespace EqualizerEffectsUnit::Internal {

    class EqualizerProcessor : public talcs::AudioSource {
    public:
        static constexpr int spectrumBinCount = equalizerSpectrumBinCount;

        struct SpectrumFrame {
            std::array<float, spectrumBinCount> levelsDb{};
        };

        EqualizerProcessor();
        ~EqualizerProcessor() override;

        void setBands(const EqualizerBandList &bands);
        void refresh();
        void setSpectrumEnabled(bool enabled);
        bool takeSpectrumFrame(SpectrumFrame &frame);
        void discardSpectrumFrames();
        double currentSampleRate() const;

        bool open(qint64 bufferSize, double sampleRate) override;
        void close() override;

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override;

    private:
        static constexpr int fftSize = 4096;
        static constexpr int fftHopSize = 1024;
        static constexpr int fftSpectrumSize = fftSize / 2;
        static constexpr std::size_t spectrumQueueCapacity = 8;
        static constexpr float spectrumFloorDb = -96.0f;

        struct AtomicBand {
            std::atomic<std::uint32_t> type{};
            std::atomic<std::uint32_t> frequencyBits{};
            std::atomic<std::uint32_t> gainBits{};
            std::atomic<std::uint32_t> qBits{};
        };

        struct ParameterSnapshot {
            std::array<EqualizerBand, maximumBandCount> bands{};
            int count{};
        };

        struct FilterBank {
            std::array<std::array<signalsmith::filters::BiquadStatic<float>, maximumBandCount>, 2> filters;
            int count{};
        };

        bool tryReadParameterSnapshot(ParameterSnapshot &snapshot,
                                      std::uint64_t &revision) const;
        void prepareParameterTransition();
        void resetProcessingState();
        void configureBank(FilterBank &bank, const ParameterSnapshot &snapshot,
                           bool resetStates);
        static void configureFilter(signalsmith::filters::BiquadStatic<float> &filter,
                                    const EqualizerBand &band, double sampleRate);
        static float processBankSample(FilterBank &bank, int channel, float input);
        static bool snapshotsStructurallyEqual(const ParameterSnapshot &left,
                                               const ParameterSnapshot &right);

        void analyzeOutput(float left, float right);
        void resetSpectrumAnalysis();
        void performSpectrumAnalysis();
        void publishSpectrumFrame(const SpectrumFrame &frame);

        std::array<AtomicBand, maximumBandCount> m_atomicBands;
        std::atomic<std::uint32_t> m_atomicBandCount{};
        std::atomic<std::uint64_t> m_parameterRevision{};
        std::atomic<std::uint64_t> m_sampleRateBits{};
        std::atomic<bool> m_refreshRequested{};
        std::array<FilterBank, 2> m_filterBanks;
        ParameterSnapshot m_appliedSnapshot;
        ParameterSnapshot m_transitionSnapshot;
        std::uint64_t m_appliedRevision{};
        std::uint64_t m_transitionRevision{};
        double m_sampleRate{44100.0};
        int m_activeBankIndex{};
        int m_transitionBankIndex{1};
        int m_transitionSamplesTotal{};
        int m_transitionSamplesRemaining{};
        bool m_hasAppliedSnapshot{};

        std::atomic<bool> m_spectrumEnabled{};
        signalsmith::spectral::WindowedFFT<float> m_fft;
        std::array<float, fftSize> m_fftRingLeft{};
        std::array<float, fftSize> m_fftRingRight{};
        std::array<float, fftSize> m_fftTimeLeft{};
        std::array<float, fftSize> m_fftTimeRight{};
        std::array<std::complex<float>, fftSpectrumSize> m_fftSpectrumLeft{};
        std::array<std::complex<float>, fftSpectrumSize> m_fftSpectrumRight{};
        std::array<SpectrumFrame, spectrumQueueCapacity> m_spectrumQueue;
        std::atomic<std::uint64_t> m_spectrumWriteIndex{};
        std::atomic<std::uint64_t> m_spectrumReadIndex{};
        float m_fftMagnitudeScale{1.0f};
        int m_fftWritePosition{};
        int m_fftSamplesSeen{};
        int m_fftSamplesSinceAnalysis{};
        bool m_spectrumAnalysisRunning{};
    };

}

#endif // DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERPROCESSOR_H
