// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CompressorProcessor.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <compressoreffectsunit/internal/CompressorParameters.h>

namespace CompressorEffectsUnit::Internal {

    CompressorProcessor::CompressorProcessor()
        : m_thresholdDb(static_cast<float>(defaultThresholdDb)),
          m_ratio(static_cast<float>(defaultRatio)),
          m_attackMilliseconds(static_cast<float>(defaultAttackMilliseconds)),
          m_releaseMilliseconds(static_cast<float>(defaultReleaseMilliseconds)) {
    }

    CompressorProcessor::~CompressorProcessor() = default;

    void CompressorProcessor::setParameters(double thresholdDb, double ratio,
                                             double attackMilliseconds, double releaseMilliseconds) {
        m_thresholdDb.store(static_cast<float>(std::clamp(
                                thresholdDb, minimumThresholdDb, maximumThresholdDb)),
                            std::memory_order_relaxed);
        m_ratio.store(static_cast<float>(std::clamp(ratio, minimumRatio, maximumRatio)),
                      std::memory_order_relaxed);
        m_attackMilliseconds.store(
            static_cast<float>(std::clamp(attackMilliseconds,
                                          minimumAttackMilliseconds,
                                          maximumAttackMilliseconds)),
            std::memory_order_relaxed);
        m_releaseMilliseconds.store(
            static_cast<float>(std::clamp(releaseMilliseconds,
                                          minimumReleaseMilliseconds,
                                          maximumReleaseMilliseconds)),
            std::memory_order_relaxed);
    }

    bool CompressorProcessor::takeMeterValues(MeterValues &values) {
        const auto readIndex = m_meterReadIndex.load(std::memory_order_relaxed);
        const auto writeIndex = m_meterWriteIndex.load(std::memory_order_acquire);
        if (readIndex == writeIndex) {
            return false;
        }
        values = m_meterQueue[readIndex % meterQueueCapacity];
        m_meterReadIndex.store(readIndex + 1, std::memory_order_release);
        return true;
    }

    void CompressorProcessor::discardMeterValues() {
        m_meterReadIndex.store(m_meterWriteIndex.load(std::memory_order_acquire),
                               std::memory_order_release);
    }

    bool CompressorProcessor::open(qint64 bufferSize, double sampleRate) {
        if (!AudioSource::open(bufferSize, sampleRate)) {
            return false;
        }
        m_sampleRate = sampleRate;
        m_envelope = 0.0f;
        return true;
    }

    void CompressorProcessor::close() {
        m_envelope = 0.0f;
        AudioSource::close();
    }

    qint64 CompressorProcessor::processReading(const talcs::AudioSourceReadData &readData) {
        const int channelCount = std::min(2, readData.buffer->channelCount());
        if (channelCount == 0 || readData.length <= 0) {
            return readData.length;
        }

        const float threshold = std::pow(
            10.0f, m_thresholdDb.load(std::memory_order_relaxed) * 0.05f);
        const float ratioInverse = 1.0f / m_ratio.load(std::memory_order_relaxed);
        const float attackCoefficient = coefficient(
            m_sampleRate, m_attackMilliseconds.load(std::memory_order_relaxed));
        const float releaseCoefficient = coefficient(
            m_sampleRate, m_releaseMilliseconds.load(std::memory_order_relaxed));

        float inputPeak = 0.0f;
        std::array<float, 2> outputPeaks{};
        float maximumGainReduction = 0.0f;

        for (qint64 position = readData.startPos;
             position < readData.startPos + readData.length; ++position) {
            const float leftInput = readData.buffer->sample(0, position);
            const float rightInput = channelCount > 1
                ? readData.buffer->sample(1, position)
                : 0.0f;
            const float detectorInput = channelCount > 1
                ? std::max(std::abs(leftInput), std::abs(rightInput))
                : std::abs(leftInput);
            inputPeak = std::max(inputPeak, detectorInput);

            const float envelopeCoefficient = detectorInput > m_envelope
                ? attackCoefficient
                : releaseCoefficient;
            m_envelope = detectorInput + envelopeCoefficient * (m_envelope - detectorInput);
            if (std::abs(m_envelope) < 1.0e-30f) {
                m_envelope = 0.0f;
            }

            const float gain = m_envelope < threshold
                ? 1.0f
                : std::pow(m_envelope / threshold, ratioInverse - 1.0f);
            const float gainReduction = gain >= 1.0f
                ? 0.0f
                : -20.0f * std::log10(std::max(gain, 1.0e-30f));
            maximumGainReduction = std::max(maximumGainReduction, gainReduction);

            const float leftOutput = leftInput * gain;
            readData.buffer->setSample(0, position, leftOutput);
            outputPeaks[0] = std::max(outputPeaks[0], std::abs(leftOutput));
            if (channelCount > 1) {
                const float rightOutput = rightInput * gain;
                readData.buffer->setSample(1, position, rightOutput);
                outputPeaks[1] = std::max(outputPeaks[1], std::abs(rightOutput));
            }
        }

        MeterValues meterValues;
        meterValues.inputLevelDb = decibelsFromGain(inputPeak);
        for (int channel = 0; channel < 2; ++channel) {
            meterValues.outputLevelDb[channel] = decibelsFromGain(outputPeaks[channel]);
            meterValues.gainReductionDb[channel] = maximumGainReduction;
        }
        publishMeterValues(meterValues);
        return readData.length;
    }

    void CompressorProcessor::publishMeterValues(const MeterValues &values) {
        const auto writeIndex = m_meterWriteIndex.load(std::memory_order_relaxed);
        const auto readIndex = m_meterReadIndex.load(std::memory_order_acquire);
        if (writeIndex - readIndex >= meterQueueCapacity) {
            return;
        }
        m_meterQueue[writeIndex % meterQueueCapacity] = values;
        m_meterWriteIndex.store(writeIndex + 1, std::memory_order_release);
    }

    float CompressorProcessor::coefficient(double sampleRate, float timeMilliseconds) {
        if (timeMilliseconds < 1.0e-3f) {
            return 0.0f;
        }
        const double exponent = -2.0 * std::numbers::pi * 1000.0
            / (sampleRate * static_cast<double>(timeMilliseconds));
        return static_cast<float>(std::exp(exponent));
    }

    float CompressorProcessor::decibelsFromGain(float gain) {
        if (gain <= 0.0f) {
            return meterFloorDb;
        }
        return std::max(meterFloorDb, 20.0f * std::log10(gain));
    }

}
