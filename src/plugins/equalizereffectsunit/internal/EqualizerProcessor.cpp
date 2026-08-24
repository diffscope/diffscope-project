// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EqualizerProcessor.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace EqualizerEffectsUnit::Internal {

    namespace {

        std::uint32_t floatBits(float value) {
            return std::bit_cast<std::uint32_t>(value);
        }

        float floatFromBits(std::uint32_t value) {
            return std::bit_cast<float>(value);
        }

    }

    EqualizerProcessor::EqualizerProcessor() {
        m_sampleRateBits.store(std::bit_cast<std::uint64_t>(m_sampleRate),
                               std::memory_order_relaxed);
    }

    EqualizerProcessor::~EqualizerProcessor() = default;

    void EqualizerProcessor::setBands(const EqualizerBandList &bands) {
        m_parameterRevision.fetch_add(1, std::memory_order_acq_rel);
        const int count = std::min(static_cast<int>(bands.size()), maximumBandCount);
        for (int index = 0; index < count; ++index) {
            const auto &band = bands.at(index);
            auto &atomicBand = m_atomicBands.at(static_cast<std::size_t>(index));
            atomicBand.type.store(static_cast<std::uint32_t>(band.type),
                                  std::memory_order_relaxed);
            atomicBand.frequencyBits.store(floatBits(static_cast<float>(band.frequencyHz)),
                                           std::memory_order_relaxed);
            atomicBand.gainBits.store(floatBits(static_cast<float>(band.gainDb)),
                                      std::memory_order_relaxed);
            atomicBand.qBits.store(floatBits(static_cast<float>(band.q)),
                                   std::memory_order_relaxed);
        }
        m_atomicBandCount.store(static_cast<std::uint32_t>(count),
                                std::memory_order_relaxed);
        m_parameterRevision.fetch_add(1, std::memory_order_release);
    }

    void EqualizerProcessor::refresh() {
        m_refreshRequested.store(true, std::memory_order_release);
    }

    void EqualizerProcessor::setSpectrumEnabled(bool enabled) {
        m_spectrumEnabled.store(enabled, std::memory_order_relaxed);
    }

    bool EqualizerProcessor::takeSpectrumFrame(SpectrumFrame &frame) {
        const auto readIndex = m_spectrumReadIndex.load(std::memory_order_relaxed);
        const auto writeIndex = m_spectrumWriteIndex.load(std::memory_order_acquire);
        if (readIndex == writeIndex) {
            return false;
        }
        frame = m_spectrumQueue[readIndex % spectrumQueueCapacity];
        m_spectrumReadIndex.store(readIndex + 1, std::memory_order_release);
        return true;
    }

    void EqualizerProcessor::discardSpectrumFrames() {
        m_spectrumReadIndex.store(m_spectrumWriteIndex.load(std::memory_order_acquire),
                                  std::memory_order_release);
    }

    double EqualizerProcessor::currentSampleRate() const {
        return std::bit_cast<double>(m_sampleRateBits.load(std::memory_order_relaxed));
    }

    bool EqualizerProcessor::open(qint64 bufferSize, double sampleRate) {
        if (!AudioSource::open(bufferSize, sampleRate)) {
            return false;
        }

        m_sampleRate = sampleRate;
        m_sampleRateBits.store(std::bit_cast<std::uint64_t>(sampleRate),
                               std::memory_order_relaxed);
        m_fft.setSize(fftSize);
        double coherentGain = 0.0;
        for (const float value : m_fft.window()) {
            coherentGain += value;
        }
        coherentGain /= static_cast<double>(fftSize);
        m_fftMagnitudeScale = coherentGain > 0.0
            ? static_cast<float>(2.0 / coherentGain)
            : 1.0f;

        m_refreshRequested.store(false, std::memory_order_relaxed);
        resetProcessingState();
        return true;
    }

    void EqualizerProcessor::close() {
        m_transitionSamplesRemaining = 0;
        m_hasAppliedSnapshot = false;
        m_refreshRequested.store(false, std::memory_order_relaxed);
        resetSpectrumAnalysis();
        AudioSource::close();
    }

    qint64 EqualizerProcessor::processReading(const talcs::AudioSourceReadData &readData) {
        if (m_refreshRequested.exchange(false, std::memory_order_acq_rel)) {
            resetProcessingState();
        }
        const int channelCount = std::min(2, readData.buffer->channelCount());
        if (channelCount == 0 || readData.length <= 0) {
            return readData.length;
        }

        if (m_transitionSamplesRemaining == 0) {
            prepareParameterTransition();
        }

        for (qint64 position = readData.startPos;
             position < readData.startPos + readData.length; ++position) {
            const float leftInput = readData.buffer->sample(0, position);
            const float rightInput = channelCount > 1
                ? readData.buffer->sample(1, position)
                : leftInput;

            float leftOutput;
            float rightOutput;
            if (m_transitionSamplesRemaining > 0) {
                auto &activeBank = m_filterBanks.at(static_cast<std::size_t>(m_activeBankIndex));
                auto &transitionBank = m_filterBanks.at(static_cast<std::size_t>(m_transitionBankIndex));
                const float alpha = 1.0f
                    - static_cast<float>(m_transitionSamplesRemaining - 1)
                        / static_cast<float>(m_transitionSamplesTotal);
                const float activeLeft = processBankSample(activeBank, 0, leftInput);
                const float nextLeft = processBankSample(transitionBank, 0, leftInput);
                leftOutput = activeLeft + (nextLeft - activeLeft) * alpha;
                if (channelCount > 1) {
                    const float activeRight = processBankSample(activeBank, 1, rightInput);
                    const float nextRight = processBankSample(transitionBank, 1, rightInput);
                    rightOutput = activeRight + (nextRight - activeRight) * alpha;
                } else {
                    rightOutput = leftOutput;
                }

                --m_transitionSamplesRemaining;
                if (m_transitionSamplesRemaining == 0) {
                    m_activeBankIndex = m_transitionBankIndex;
                    m_transitionBankIndex = 1 - m_activeBankIndex;
                    m_appliedSnapshot = m_transitionSnapshot;
                    m_appliedRevision = m_transitionRevision;
                }
            } else {
                auto &activeBank = m_filterBanks.at(static_cast<std::size_t>(m_activeBankIndex));
                leftOutput = processBankSample(activeBank, 0, leftInput);
                rightOutput = channelCount > 1
                    ? processBankSample(activeBank, 1, rightInput)
                    : leftOutput;
            }

            readData.buffer->setSample(0, position, leftOutput);
            if (channelCount > 1) {
                readData.buffer->setSample(1, position, rightOutput);
            }
            analyzeOutput(leftOutput, rightOutput);
        }
        return readData.length;
    }

    bool EqualizerProcessor::tryReadParameterSnapshot(ParameterSnapshot &snapshot,
                                                      std::uint64_t &revision) const {
        const auto firstRevision = m_parameterRevision.load(std::memory_order_acquire);
        if (firstRevision % 2 != 0) {
            return false;
        }

        const int count = std::min(
            static_cast<int>(m_atomicBandCount.load(std::memory_order_relaxed)),
            maximumBandCount);
        snapshot.count = count;
        for (int index = 0; index < count; ++index) {
            const auto &atomicBand = m_atomicBands.at(static_cast<std::size_t>(index));
            auto &band = snapshot.bands.at(static_cast<std::size_t>(index));
            const auto typeValue = atomicBand.type.load(std::memory_order_relaxed);
            band.type = typeValue <= static_cast<std::uint32_t>(EqualizerBandType::HighShelf)
                ? static_cast<EqualizerBandType>(typeValue)
                : EqualizerBandType::Bell;
            band.frequencyHz = floatFromBits(
                atomicBand.frequencyBits.load(std::memory_order_relaxed));
            band.gainDb = floatFromBits(
                atomicBand.gainBits.load(std::memory_order_relaxed));
            band.q = floatFromBits(atomicBand.qBits.load(std::memory_order_relaxed));
        }

        const auto secondRevision = m_parameterRevision.load(std::memory_order_acquire);
        if (firstRevision != secondRevision || secondRevision % 2 != 0) {
            return false;
        }
        revision = secondRevision;
        return true;
    }

    void EqualizerProcessor::prepareParameterTransition() {
        ParameterSnapshot snapshot;
        std::uint64_t revision;
        if (!tryReadParameterSnapshot(snapshot, revision)
            || m_hasAppliedSnapshot && revision == m_appliedRevision) {
            return;
        }

        if (!m_hasAppliedSnapshot) {
            configureBank(m_filterBanks.at(static_cast<std::size_t>(m_activeBankIndex)),
                          snapshot, true);
            m_appliedSnapshot = snapshot;
            m_appliedRevision = revision;
            m_hasAppliedSnapshot = true;
            return;
        }

        auto &activeBank = m_filterBanks.at(static_cast<std::size_t>(m_activeBankIndex));
        auto &transitionBank = m_filterBanks.at(static_cast<std::size_t>(m_transitionBankIndex));
        transitionBank = activeBank;
        configureBank(transitionBank, snapshot,
                      !snapshotsStructurallyEqual(m_appliedSnapshot, snapshot));
        m_transitionSnapshot = snapshot;
        m_transitionRevision = revision;
        m_transitionSamplesTotal = std::max(1, static_cast<int>(m_sampleRate * 0.005));
        m_transitionSamplesRemaining = m_transitionSamplesTotal;
    }

    void EqualizerProcessor::resetProcessingState() {
        for (auto &bank : m_filterBanks) {
            bank.count = 0;
            for (auto &channel : bank.filters) {
                for (auto &filter : channel) {
                    filter.reset();
                }
            }
        }
        m_activeBankIndex = 0;
        m_transitionBankIndex = 1;
        m_transitionSamplesRemaining = 0;
        m_hasAppliedSnapshot = false;
        prepareParameterTransition();
        resetSpectrumAnalysis();
    }

    void EqualizerProcessor::configureBank(FilterBank &bank,
                                           const ParameterSnapshot &snapshot,
                                           bool resetStates) {
        if (resetStates) {
            for (auto &channel : bank.filters) {
                for (auto &filter : channel) {
                    filter.reset();
                }
            }
        }
        bank.count = snapshot.count;
        for (int channel = 0; channel < 2; ++channel) {
            for (int index = 0; index < snapshot.count; ++index) {
                configureFilter(bank.filters.at(static_cast<std::size_t>(channel))
                                    .at(static_cast<std::size_t>(index)),
                                snapshot.bands.at(static_cast<std::size_t>(index)),
                                m_sampleRate);
            }
        }
    }

    void EqualizerProcessor::configureFilter(
        signalsmith::filters::BiquadStatic<float> &filter,
        const EqualizerBand &band, double sampleRate) {
        const double scaledFrequency = std::clamp(
            band.frequencyHz / sampleRate, 1.0e-6, 0.499);
        using signalsmith::filters::BiquadDesign;
        switch (band.type) {
            case EqualizerBandType::LowShelf:
                filter.lowShelfDbQ(scaledFrequency, band.gainDb, band.q,
                                   BiquadDesign::oneSided);
                break;
            case EqualizerBandType::HighShelf:
                filter.highShelfDbQ(scaledFrequency, band.gainDb, band.q,
                                    BiquadDesign::oneSided);
                break;
            case EqualizerBandType::Bell:
            default:
                filter.peakDbQ(scaledFrequency, band.gainDb, band.q,
                               BiquadDesign::oneSided);
                break;
        }
    }

    float EqualizerProcessor::processBankSample(FilterBank &bank, int channel,
                                                float input) {
        float output = input;
        auto &filters = bank.filters.at(static_cast<std::size_t>(channel));
        for (int index = 0; index < bank.count; ++index) {
            output = filters.at(static_cast<std::size_t>(index))(output);
        }
        return output;
    }

    bool EqualizerProcessor::snapshotsStructurallyEqual(
        const ParameterSnapshot &left, const ParameterSnapshot &right) {
        if (left.count != right.count) {
            return false;
        }
        for (int index = 0; index < left.count; ++index) {
            if (left.bands.at(static_cast<std::size_t>(index)).type
                != right.bands.at(static_cast<std::size_t>(index)).type) {
                return false;
            }
        }
        return true;
    }

    void EqualizerProcessor::analyzeOutput(float left, float right) {
        const bool enabled = m_spectrumEnabled.load(std::memory_order_relaxed);
        if (!enabled) {
            if (m_spectrumAnalysisRunning) {
                resetSpectrumAnalysis();
            }
            return;
        }
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
            performSpectrumAnalysis();
        }
    }

    void EqualizerProcessor::resetSpectrumAnalysis() {
        m_fftWritePosition = 0;
        m_fftSamplesSeen = 0;
        m_fftSamplesSinceAnalysis = 0;
        m_spectrumAnalysisRunning = false;
        m_fftRingLeft.fill(0.0f);
        m_fftRingRight.fill(0.0f);
    }

    void EqualizerProcessor::performSpectrumAnalysis() {
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
                frame.levelsDb.at(static_cast<std::size_t>(index)) = spectrumFloorDb;
                continue;
            }

            const int lowerIndex = static_cast<int>(std::floor(fftPosition));
            const int upperIndex = lowerIndex + 1;
            const float fraction = static_cast<float>(fftPosition - lowerIndex);
            const auto powerAt = [this, magnitudeScaleSquared](
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
            const float power = std::max((leftPower + rightPower) * 0.5f, 1.0e-30f);
            frame.levelsDb.at(static_cast<std::size_t>(index)) = std::clamp(
                10.0f * std::log10(power), spectrumFloorDb, 0.0f);
        }
        publishSpectrumFrame(frame);
    }

    void EqualizerProcessor::publishSpectrumFrame(const SpectrumFrame &frame) {
        const auto writeIndex = m_spectrumWriteIndex.load(std::memory_order_relaxed);
        const auto readIndex = m_spectrumReadIndex.load(std::memory_order_acquire);
        if (writeIndex - readIndex >= spectrumQueueCapacity) {
            return;
        }
        m_spectrumQueue[writeIndex % spectrumQueueCapacity] = frame;
        m_spectrumWriteIndex.store(writeIndex + 1, std::memory_order_release);
    }

}
