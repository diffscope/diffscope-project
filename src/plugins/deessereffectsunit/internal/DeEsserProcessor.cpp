// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DeEsserProcessor.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <complex>
#include <numbers>

#include <signalsmith-dsp/filters.h>
#include <signalsmith-dsp/spectral.h>

namespace DeEsserEffectsUnit::Internal {

    namespace {

        constexpr int fftSize = 4096;
        constexpr int fftHopSize = 1024;
        constexpr int fftSpectrumSize = fftSize / 2;
        constexpr double parameterTransitionSeconds = 0.005;
        constexpr float denormalThreshold = 1.0e-30f;

        std::uint32_t floatBits(float value) {
            return std::bit_cast<std::uint32_t>(value);
        }

        float floatFromBits(std::uint32_t value) {
            return std::bit_cast<float>(value);
        }

        float normalizedParameter(double value, double fallback,
                                  double minimum, double maximum) {
            if (!std::isfinite(value)) {
                value = fallback;
            }
            return static_cast<float>(std::clamp(value, minimum, maximum));
        }

        bool valuesEqual(float left, float right) {
            return std::abs(left - right) <= 1.0e-6f;
        }

        float withoutDenormal(float value) {
            return std::abs(value) < denormalThreshold ? 0.0f : value;
        }

        float envelopeCoefficient(double sampleRate, double milliseconds) {
            const double exponent = -2.0 * std::numbers::pi * 1000.0
                / (sampleRate * milliseconds);
            return static_cast<float>(std::exp(exponent));
        }

        float decibelsFromGain(float gain) {
            if (gain <= 0.0f) {
                return meterFloorDb;
            }
            return std::max(meterFloorDb, 20.0f * std::log10(gain));
        }

    }

    class DeEsserProcessor::ProcessingEngine {
    public:
        void configure(double sampleRate, const ParameterSnapshot &parameters) {
            m_sampleRate = sampleRate;
            m_transitionSamples = std::max(
                1, static_cast<int>(std::round(sampleRate * parameterTransitionSeconds)));
            m_attackCoefficient = envelopeCoefficient(
                sampleRate, compressorAttackMilliseconds);
            m_releaseCoefficient = envelopeCoefficient(
                sampleRate, compressorReleaseMilliseconds);

            m_activeBank = 0;
            m_transitionBank = 1;
            m_transitionSamplesRemaining = 0;
            m_currentParameters = parameters;
            m_requestedParameters = parameters;
            m_transitionParameters = parameters;
            configureBank(m_filterBanks.at(0), parameters, true);
            m_filterBanks.at(1) = m_filterBanks.at(0);

            m_fft.setSize(fftSize);
            double coherentGain = 0.0;
            for (const float value : m_fft.window()) {
                coherentGain += value;
            }
            coherentGain /= static_cast<double>(fftSize);
            m_fftMagnitudeScale = coherentGain > 0.0
                ? static_cast<float>(2.0 / coherentGain)
                : 1.0f;
            refresh();
            m_configured = true;
        }

        void setParameters(const ParameterSnapshot &parameters) {
            m_requestedParameters = parameters;
            if (m_transitionSamplesRemaining == 0) {
                startParameterTransition();
            }
        }

        void refresh() {
            for (auto &bank : m_filterBanks) {
                for (auto &filter : bank) {
                    filter.reset();
                }
            }
            m_envelope = 0.0f;
            resetSpectrumAnalysis();
        }

        void close() {
            refresh();
            m_transitionSamplesRemaining = 0;
            m_configured = false;
        }

        void process(talcs::IAudioSampleContainer &buffer, qint64 startPosition,
                     qint64 length, bool analysisEnabled,
                     DeEsserProcessor &processor) {
            if (!m_configured || length <= 0) {
                return;
            }
            const int channelCount = std::min(2, buffer.channelCount());
            if (channelCount == 0) {
                return;
            }

            std::array<float, 2> bandPeaks{};
            float minimumGain = 1.0f;
            for (qint64 position = startPosition;
                 position < startPosition + length; ++position) {
                std::array<float, 2> input{
                    buffer.sample(0, position),
                    channelCount > 1 ? buffer.sample(1, position) : 0.0f,
                };
                std::array<float, 2> band{};
                float thresholdDb = m_currentParameters.thresholdDb;
                float sibilanceMix = m_currentParameters.outputSibilanceOnly
                    ? 1.0f
                    : 0.0f;

                if (m_transitionSamplesRemaining > 0) {
                    const float alpha = 1.0f
                        - static_cast<float>(m_transitionSamplesRemaining - 1)
                            / static_cast<float>(m_transitionSamples);
                    for (int channel = 0; channel < channelCount; ++channel) {
                        const auto index = static_cast<std::size_t>(channel);
                        const float activeValue = m_filterBanks.at(
                            static_cast<std::size_t>(m_activeBank)).at(index)(input.at(index));
                        const float transitionValue = m_filterBanks.at(
                            static_cast<std::size_t>(m_transitionBank)).at(index)(input.at(index));
                        band.at(index) = std::lerp(activeValue, transitionValue, alpha);
                    }
                    thresholdDb = std::lerp(m_currentParameters.thresholdDb,
                                            m_transitionParameters.thresholdDb,
                                            alpha);
                    sibilanceMix = std::lerp(
                        m_currentParameters.outputSibilanceOnly ? 1.0f : 0.0f,
                        m_transitionParameters.outputSibilanceOnly ? 1.0f : 0.0f,
                        alpha);
                } else {
                    for (int channel = 0; channel < channelCount; ++channel) {
                        const auto index = static_cast<std::size_t>(channel);
                        band.at(index) = m_filterBanks.at(
                            static_cast<std::size_t>(m_activeBank)).at(index)(input.at(index));
                    }
                }

                const float detectorInput = channelCount > 1
                    ? std::max(std::abs(band[0]), std::abs(band[1]))
                    : std::abs(band[0]);
                const float coefficient = detectorInput > m_envelope
                    ? m_attackCoefficient
                    : m_releaseCoefficient;
                m_envelope = withoutDenormal(
                    detectorInput + coefficient * (m_envelope - detectorInput));

                const float envelopeDb = m_envelope > 0.0f
                    ? 20.0f * std::log10(m_envelope)
                    : meterFloorDb;
                const float compressionDb = std::clamp(
                    thresholdDb - envelopeDb,
                    static_cast<float>(maximumCompressionDb), 0.0f);
                const float gain = std::pow(10.0f, compressionDb * 0.05f);
                minimumGain = std::min(minimumGain, gain);

                std::array<float, 2> output{};
                for (int channel = 0; channel < channelCount; ++channel) {
                    const auto index = static_cast<std::size_t>(channel);
                    bandPeaks.at(index) = std::max(
                        bandPeaks.at(index), std::abs(band.at(index)));
                    const float deEssed = input.at(index)
                        + band.at(index) * (gain - 1.0f);
                    output.at(index) = std::lerp(deEssed, band.at(index), sibilanceMix);
                    buffer.setSample(channel, position, output.at(index));
                }

                if (analysisEnabled) {
                    analyzeOutput(output[0], channelCount > 1 ? output[1] : output[0],
                                  processor);
                } else if (m_spectrumAnalysisRunning) {
                    resetSpectrumAnalysis();
                }
                advanceParameterTransition();
            }

            if (analysisEnabled) {
                MeterValues values;
                const float gainReduction = minimumGain >= 1.0f
                    ? 0.0f
                    : -20.0f * std::log10(std::max(minimumGain, denormalThreshold));
                for (int channel = 0; channel < 2; ++channel) {
                    values.bandLevelDb.at(static_cast<std::size_t>(channel)) =
                        decibelsFromGain(bandPeaks.at(static_cast<std::size_t>(channel)));
                    values.gainReductionDb.at(static_cast<std::size_t>(channel)) =
                        channel < channelCount ? gainReduction : 0.0f;
                }
                processor.publishMeterValues(values);
            }
        }

    private:
        using Filter = signalsmith::filters::BiquadStatic<float>;
        using FilterBank = std::array<Filter, 2>;

        static bool parametersEqual(const ParameterSnapshot &left,
                                    const ParameterSnapshot &right) {
            return valuesEqual(left.frequencyHz, right.frequencyHz)
                && valuesEqual(left.bandwidthHz, right.bandwidthHz)
                && valuesEqual(left.thresholdDb, right.thresholdDb)
                && left.outputSibilanceOnly == right.outputSibilanceOnly;
        }

        void configureBank(FilterBank &bank, const ParameterSnapshot &parameters,
                           bool resetStates) {
            const double scaledFrequency = std::clamp(
                static_cast<double>(parameters.frequencyHz) / m_sampleRate,
                1.0e-6, 0.499);
            const double q = std::max(
                1.0e-3, static_cast<double>(parameters.frequencyHz)
                    / static_cast<double>(parameters.bandwidthHz));
            for (auto &filter : bank) {
                if (resetStates) {
                    filter.reset();
                }
                filter.bandpassQ(scaledFrequency, q,
                                 signalsmith::filters::BiquadDesign::oneSided);
            }
        }

        void startParameterTransition() {
            if (parametersEqual(m_currentParameters, m_requestedParameters)) {
                return;
            }
            m_transitionBank = 1 - m_activeBank;
            m_filterBanks.at(static_cast<std::size_t>(m_transitionBank)) =
                m_filterBanks.at(static_cast<std::size_t>(m_activeBank));
            configureBank(m_filterBanks.at(static_cast<std::size_t>(m_transitionBank)),
                          m_requestedParameters, false);
            m_transitionParameters = m_requestedParameters;
            m_transitionSamplesRemaining = m_transitionSamples;
        }

        void advanceParameterTransition() {
            if (m_transitionSamplesRemaining == 0) {
                return;
            }
            --m_transitionSamplesRemaining;
            if (m_transitionSamplesRemaining > 0) {
                return;
            }
            m_activeBank = m_transitionBank;
            m_currentParameters = m_transitionParameters;
            if (!parametersEqual(m_currentParameters, m_requestedParameters)) {
                startParameterTransition();
            }
        }

        void analyzeOutput(float left, float right,
                           DeEsserProcessor &processor) {
            if (!m_spectrumAnalysisRunning) {
                resetSpectrumAnalysis();
                m_spectrumAnalysisRunning = true;
            }
            m_fftRingLeft.at(static_cast<std::size_t>(m_fftWritePosition)) = left;
            m_fftRingRight.at(static_cast<std::size_t>(m_fftWritePosition)) = right;
            m_fftWritePosition = (m_fftWritePosition + 1) % fftSize;
            m_fftSamplesSeen = std::min(m_fftSamplesSeen + 1, fftSize);
            ++m_fftSamplesSinceAnalysis;
            if (m_fftSamplesSeen == fftSize
                && m_fftSamplesSinceAnalysis >= fftHopSize) {
                m_fftSamplesSinceAnalysis = 0;
                performSpectrumAnalysis(processor);
            }
        }

        void resetSpectrumAnalysis() {
            m_fftWritePosition = 0;
            m_fftSamplesSeen = 0;
            m_fftSamplesSinceAnalysis = 0;
            m_spectrumAnalysisRunning = false;
            m_fftRingLeft.fill(0.0f);
            m_fftRingRight.fill(0.0f);
        }

        void performSpectrumAnalysis(DeEsserProcessor &processor) {
            for (int index = 0; index < fftSize; ++index) {
                const int ringIndex = (m_fftWritePosition + index) % fftSize;
                m_fftTimeLeft.at(static_cast<std::size_t>(index)) =
                    m_fftRingLeft.at(static_cast<std::size_t>(ringIndex));
                m_fftTimeRight.at(static_cast<std::size_t>(index)) =
                    m_fftRingRight.at(static_cast<std::size_t>(ringIndex));
            }
            m_fft.fft<true, true>(m_fftTimeLeft, m_fftSpectrumLeft);
            m_fft.fft<true, true>(m_fftTimeRight, m_fftSpectrumRight);

            SpectrumFrame frame;
            const float magnitudeScaleSquared = m_fftMagnitudeScale * m_fftMagnitudeScale;
            for (int index = 0; index < spectrumBinCount; ++index) {
                const double normalizedPosition = static_cast<double>(index)
                    / static_cast<double>(spectrumBinCount - 1);
                const double frequency = minimumFrequencyHz
                    * std::pow(maximumFrequencyHz / minimumFrequencyHz,
                               normalizedPosition);
                const double fftPosition = frequency * static_cast<double>(fftSize)
                        / m_sampleRate
                    - 0.5;
                if (fftPosition < 0.0 || fftPosition >= fftSpectrumSize - 1) {
                    frame.levelsDb.at(static_cast<std::size_t>(index)) = meterFloorDb;
                    continue;
                }

                const int lowerIndex = static_cast<int>(std::floor(fftPosition));
                const int upperIndex = lowerIndex + 1;
                const float fraction = static_cast<float>(fftPosition - lowerIndex);
                const auto powerAt = [magnitudeScaleSquared](
                                         const std::array<std::complex<float>, fftSpectrumSize> &spectrum,
                                         int spectrumIndex) {
                    return std::norm(spectrum.at(static_cast<std::size_t>(spectrumIndex)))
                        * magnitudeScaleSquared;
                };
                const float leftPower = std::lerp(
                    powerAt(m_fftSpectrumLeft, lowerIndex),
                    powerAt(m_fftSpectrumLeft, upperIndex), fraction);
                const float rightPower = std::lerp(
                    powerAt(m_fftSpectrumRight, lowerIndex),
                    powerAt(m_fftSpectrumRight, upperIndex), fraction);
                const float power = std::max(
                    (leftPower + rightPower) * 0.5f, denormalThreshold);
                frame.levelsDb.at(static_cast<std::size_t>(index)) = std::clamp(
                    10.0f * std::log10(power), meterFloorDb, 0.0f);
            }
            processor.publishSpectrumFrame(frame);
        }

        std::array<FilterBank, 2> m_filterBanks;
        ParameterSnapshot m_currentParameters;
        ParameterSnapshot m_requestedParameters;
        ParameterSnapshot m_transitionParameters;
        signalsmith::spectral::WindowedFFT<float> m_fft;
        std::array<float, fftSize> m_fftRingLeft{};
        std::array<float, fftSize> m_fftRingRight{};
        std::array<float, fftSize> m_fftTimeLeft{};
        std::array<float, fftSize> m_fftTimeRight{};
        std::array<std::complex<float>, fftSpectrumSize> m_fftSpectrumLeft{};
        std::array<std::complex<float>, fftSpectrumSize> m_fftSpectrumRight{};
        double m_sampleRate{44100.0};
        float m_attackCoefficient{};
        float m_releaseCoefficient{};
        float m_envelope{};
        float m_fftMagnitudeScale{1.0f};
        int m_activeBank{};
        int m_transitionBank{1};
        int m_transitionSamples{1};
        int m_transitionSamplesRemaining{};
        int m_fftWritePosition{};
        int m_fftSamplesSeen{};
        int m_fftSamplesSinceAnalysis{};
        bool m_spectrumAnalysisRunning{};
        bool m_configured{};
    };

    DeEsserProcessor::DeEsserProcessor()
        : m_engine(std::make_unique<ProcessingEngine>()) {
        m_sampleRateBits.store(std::bit_cast<std::uint64_t>(44100.0),
                               std::memory_order_relaxed);
        setParameters(defaultFrequencyHz, defaultBandwidthHz,
                      defaultThresholdDb, defaultOutputSibilanceOnly);
    }

    DeEsserProcessor::~DeEsserProcessor() = default;

    void DeEsserProcessor::setParameters(double frequencyHz, double bandwidthHz,
                                         double thresholdDb,
                                         bool outputSibilanceOnly) {
        const float normalizedFrequency = normalizedParameter(
            frequencyHz, defaultFrequencyHz,
            minimumFrequencyHz, maximumFrequencyHz);
        const float normalizedBandwidth = normalizedParameter(
            bandwidthHz, defaultBandwidthHz,
            minimumBandwidthHz, maximumBandwidthHz);
        const float normalizedThreshold = normalizedParameter(
            thresholdDb, defaultThresholdDb,
            minimumThresholdDb, maximumThresholdDb);

        m_parameterRevision.fetch_add(1, std::memory_order_acq_rel);
        m_frequencyBits.store(floatBits(normalizedFrequency),
                              std::memory_order_relaxed);
        m_bandwidthBits.store(floatBits(normalizedBandwidth),
                              std::memory_order_relaxed);
        m_thresholdBits.store(floatBits(normalizedThreshold),
                              std::memory_order_relaxed);
        m_outputSibilanceOnly.store(outputSibilanceOnly,
                                    std::memory_order_relaxed);
        m_parameterRevision.fetch_add(1, std::memory_order_release);
    }

    void DeEsserProcessor::refresh() {
        m_refreshRequested.store(true, std::memory_order_release);
    }

    void DeEsserProcessor::setAnalysisEnabled(bool enabled) {
        m_analysisEnabled.store(enabled, std::memory_order_relaxed);
    }

    bool DeEsserProcessor::takeMeterValues(MeterValues &values) {
        const auto readIndex = m_meterReadIndex.load(std::memory_order_relaxed);
        const auto writeIndex = m_meterWriteIndex.load(std::memory_order_acquire);
        if (readIndex == writeIndex) {
            return false;
        }
        values = m_meterQueue[readIndex % meterQueueCapacity];
        m_meterReadIndex.store(readIndex + 1, std::memory_order_release);
        return true;
    }

    void DeEsserProcessor::discardMeterValues() {
        m_meterReadIndex.store(m_meterWriteIndex.load(std::memory_order_acquire),
                               std::memory_order_release);
    }

    bool DeEsserProcessor::takeSpectrumFrame(SpectrumFrame &frame) {
        const auto readIndex = m_spectrumReadIndex.load(std::memory_order_relaxed);
        const auto writeIndex = m_spectrumWriteIndex.load(std::memory_order_acquire);
        if (readIndex == writeIndex) {
            return false;
        }
        frame = m_spectrumQueue[readIndex % spectrumQueueCapacity];
        m_spectrumReadIndex.store(readIndex + 1, std::memory_order_release);
        return true;
    }

    void DeEsserProcessor::discardSpectrumFrames() {
        m_spectrumReadIndex.store(m_spectrumWriteIndex.load(std::memory_order_acquire),
                                  std::memory_order_release);
    }

    double DeEsserProcessor::currentSampleRate() const {
        return std::bit_cast<double>(
            m_sampleRateBits.load(std::memory_order_relaxed));
    }

    bool DeEsserProcessor::open(qint64 bufferSize, double sampleRate) {
        if (!AudioSource::open(bufferSize, sampleRate)) {
            return false;
        }

        m_sampleRateBits.store(std::bit_cast<std::uint64_t>(sampleRate),
                               std::memory_order_relaxed);
        ParameterSnapshot snapshot{
            static_cast<float>(defaultFrequencyHz),
            static_cast<float>(defaultBandwidthHz),
            static_cast<float>(defaultThresholdDb),
            defaultOutputSibilanceOnly,
        };
        std::uint64_t revision{};
        if (tryReadParameterSnapshot(snapshot, revision)) {
            m_appliedRevision = revision;
        } else {
            m_appliedRevision = 0;
        }
        m_engine->configure(sampleRate, snapshot);
        m_refreshRequested.store(false, std::memory_order_relaxed);
        return true;
    }

    void DeEsserProcessor::close() {
        m_engine->close();
        m_appliedRevision = 0;
        m_refreshRequested.store(false, std::memory_order_relaxed);
        AudioSource::close();
    }

    qint64 DeEsserProcessor::processReading(
        const talcs::AudioSourceReadData &readData) {
        if (m_refreshRequested.exchange(false, std::memory_order_acq_rel)) {
            m_engine->refresh();
        }
        ParameterSnapshot snapshot;
        std::uint64_t revision{};
        if (tryReadParameterSnapshot(snapshot, revision)
            && revision != m_appliedRevision) {
            m_engine->setParameters(snapshot);
            m_appliedRevision = revision;
        }
        m_engine->process(*readData.buffer, readData.startPos, readData.length,
                          m_analysisEnabled.load(std::memory_order_relaxed), *this);
        return readData.length;
    }

    bool DeEsserProcessor::tryReadParameterSnapshot(
        ParameterSnapshot &snapshot, std::uint64_t &revision) const {
        const auto firstRevision = m_parameterRevision.load(std::memory_order_acquire);
        if (firstRevision % 2 != 0) {
            return false;
        }

        snapshot.frequencyHz = floatFromBits(
            m_frequencyBits.load(std::memory_order_relaxed));
        snapshot.bandwidthHz = floatFromBits(
            m_bandwidthBits.load(std::memory_order_relaxed));
        snapshot.thresholdDb = floatFromBits(
            m_thresholdBits.load(std::memory_order_relaxed));
        snapshot.outputSibilanceOnly = m_outputSibilanceOnly.load(
            std::memory_order_relaxed);

        const auto secondRevision = m_parameterRevision.load(std::memory_order_acquire);
        if (firstRevision != secondRevision || secondRevision % 2 != 0) {
            return false;
        }
        revision = secondRevision;
        return true;
    }

    void DeEsserProcessor::publishMeterValues(const MeterValues &values) {
        const auto writeIndex = m_meterWriteIndex.load(std::memory_order_relaxed);
        const auto readIndex = m_meterReadIndex.load(std::memory_order_acquire);
        if (writeIndex - readIndex >= meterQueueCapacity) {
            return;
        }
        m_meterQueue[writeIndex % meterQueueCapacity] = values;
        m_meterWriteIndex.store(writeIndex + 1, std::memory_order_release);
    }

    void DeEsserProcessor::publishSpectrumFrame(const SpectrumFrame &frame) {
        const auto writeIndex = m_spectrumWriteIndex.load(std::memory_order_relaxed);
        const auto readIndex = m_spectrumReadIndex.load(std::memory_order_acquire);
        if (writeIndex - readIndex >= spectrumQueueCapacity) {
            return;
        }
        m_spectrumQueue[writeIndex % spectrumQueueCapacity] = frame;
        m_spectrumWriteIndex.store(writeIndex + 1, std::memory_order_release);
    }

}
