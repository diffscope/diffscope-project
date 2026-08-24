// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ReverbProcessor.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>

#include <reverbeffectsunit/internal/ReverbParameters.h>

#include <signalsmith-dsp/delay.h>
#include <signalsmith-dsp/filters.h>
#include <signalsmith-dsp/mix.h>

namespace ReverbEffectsUnit::Internal {

    namespace {

        constexpr int internalChannelCount = 8;
        constexpr int diffusionStepCount = 4;
        constexpr double parameterTransitionSeconds = 0.02;
        constexpr double dampingShelfFrequencyHz = 4000.0;
        constexpr float denormalThreshold = 1.0e-30f;

        using InternalArray = std::array<float, internalChannelCount>;
        using StereoArray = std::array<float, 2>;
        using Delay = signalsmith::delay::Delay<float>;
        using DampingFilter = signalsmith::filters::BiquadStatic<float>;
        using StereoMixer = signalsmith::mix::StereoMultiMixer<
            float, internalChannelCount>;

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

        class LinearRamp {
        public:
            void reset(float value) {
                m_current = value;
                m_target = value;
                m_step = 0.0f;
                m_remaining = 0;
            }

            void setTarget(float value, int sampleCount) {
                if (valuesEqual(value, m_target)) {
                    return;
                }
                m_target = value;
                m_remaining = std::max(1, sampleCount);
                m_step = (m_target - m_current) / static_cast<float>(m_remaining);
            }

            float next() {
                if (m_remaining > 0) {
                    m_current += m_step;
                    --m_remaining;
                    if (m_remaining == 0) {
                        m_current = m_target;
                    }
                }
                return m_current;
            }

        private:
            float m_current{};
            float m_target{};
            float m_step{};
            int m_remaining{};
        };

        class TapTransition {
        public:
            void reset(float value) {
                m_current = value;
                m_from = value;
                m_to = value;
                m_requested = value;
                m_total = 0;
                m_remaining = 0;
            }

            void request(float value, int sampleCount) {
                if (valuesEqual(value, m_requested)) {
                    return;
                }
                m_requested = value;
                m_duration = std::max(1, sampleCount);
                if (!active()) {
                    startRequestedTransition();
                }
            }

            bool active() const {
                return m_remaining > 0;
            }

            float from() const {
                return active() ? m_from : m_current;
            }

            float to() const {
                return active() ? m_to : m_current;
            }

            float alpha() const {
                if (!active()) {
                    return 0.0f;
                }
                return 1.0f - static_cast<float>(m_remaining)
                    / static_cast<float>(m_total);
            }

            void advance() {
                if (!active()) {
                    return;
                }
                --m_remaining;
                if (m_remaining > 0) {
                    return;
                }
                m_current = m_to;
                m_from = m_current;
                if (!valuesEqual(m_current, m_requested)) {
                    startRequestedTransition();
                }
            }

        private:
            void startRequestedTransition() {
                if (valuesEqual(m_current, m_requested)) {
                    return;
                }
                m_from = m_current;
                m_to = m_requested;
                m_total = m_duration;
                m_remaining = m_total;
            }

            float m_current{};
            float m_from{};
            float m_to{};
            float m_requested{};
            int m_duration{1};
            int m_total{};
            int m_remaining{};
        };

        struct DiffusionStep {
            std::array<Delay, internalChannelCount> delays;
            std::array<float, internalChannelCount> delayRatios{};
            std::array<bool, internalChannelCount> flipPolarity{};
            std::array<int, internalChannelCount> shuffle{};

            void configure(int stepIndex, double sampleRate,
                           std::minstd_rand &random) {
                const float stageRatio = std::pow(0.5f, static_cast<float>(stepIndex + 1));
                const int maximumDelaySamples = static_cast<int>(std::ceil(
                    maximumSizeMilliseconds * 0.001 * sampleRate * stageRatio)) + 2;
                std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
                std::bernoulli_distribution polarityDistribution(0.5);
                for (int channel = 0; channel < internalChannelCount; ++channel) {
                    const float segmentPosition =
                        (static_cast<float>(channel) + distribution(random))
                        / static_cast<float>(internalChannelCount);
                    delayRatios.at(static_cast<std::size_t>(channel)) =
                        stageRatio * segmentPosition;
                    flipPolarity.at(static_cast<std::size_t>(channel)) =
                        polarityDistribution(random);
                    auto &delay = delays.at(static_cast<std::size_t>(channel));
                    delay.resize(maximumDelaySamples);
                    delay.reset();
                    shuffle.at(static_cast<std::size_t>(channel)) = channel;
                }
                for (int channel = internalChannelCount - 1; channel > 0; --channel) {
                    std::uniform_int_distribution<int> shuffleDistribution(0, channel);
                    const int other = shuffleDistribution(random);
                    std::swap(shuffle.at(static_cast<std::size_t>(channel)),
                              shuffle.at(static_cast<std::size_t>(other)));
                }
            }

            void reset() {
                for (auto &delay : delays) {
                    delay.reset();
                }
            }

            InternalArray process(const InternalArray &input,
                                  float sizeSamplesFrom, float sizeSamplesTo,
                                  float transitionAlpha) {
                InternalArray delayed;
                for (int channel = 0; channel < internalChannelCount; ++channel) {
                    auto &delay = delays.at(static_cast<std::size_t>(channel));
                    delay.write(input.at(static_cast<std::size_t>(channel)));
                    const float ratio = delayRatios.at(static_cast<std::size_t>(channel));
                    const float fromValue = delay.read(sizeSamplesFrom * ratio);
                    const float toValue = delay.read(sizeSamplesTo * ratio);
                    delayed.at(static_cast<std::size_t>(channel)) =
                        std::lerp(fromValue, toValue, transitionAlpha);
                }
                InternalArray shuffled;
                for (int channel = 0; channel < internalChannelCount; ++channel) {
                    const auto index = static_cast<std::size_t>(channel);
                    float value = delayed.at(static_cast<std::size_t>(
                        shuffle.at(index)));
                    shuffled.at(index) = flipPolarity.at(index) ? -value : value;
                }
                signalsmith::mix::Hadamard<float, internalChannelCount>::inPlace(shuffled);
                return shuffled;
            }
        };

        struct DampingConfiguration {
            float sizeMilliseconds{};
            float decaySeconds{};
            float dampingPercent{};
        };

        bool configurationsEqual(const DampingConfiguration &left,
                                 const DampingConfiguration &right) {
            return valuesEqual(left.sizeMilliseconds, right.sizeMilliseconds)
                && valuesEqual(left.decaySeconds, right.decaySeconds)
                && valuesEqual(left.dampingPercent, right.dampingPercent);
        }

    }

    class ReverbProcessor::ReverbEngine {
    public:
        void configure(double sampleRate, const ParameterSnapshot &parameters) {
            m_sampleRate = sampleRate;
            m_transitionSamples = std::max(
                1, static_cast<int>(std::round(sampleRate * parameterTransitionSeconds)));

            const int maximumPreDelaySamples = static_cast<int>(std::ceil(
                maximumPreDelayMilliseconds * 0.001 * sampleRate)) + 2;
            for (auto &delay : m_preDelayLines) {
                delay.resize(maximumPreDelaySamples);
                delay.reset();
            }

            std::minstd_rand random(0x4D595DF4u);
            for (int step = 0; step < diffusionStepCount; ++step) {
                m_diffusionSteps.at(static_cast<std::size_t>(step))
                    .configure(step, sampleRate, random);
            }

            const int maximumFeedbackDelaySamples = static_cast<int>(std::ceil(
                maximumSizeMilliseconds * 0.002 * sampleRate)) + 2;
            for (int channel = 0; channel < internalChannelCount; ++channel) {
                m_feedbackDelayRatios.at(static_cast<std::size_t>(channel)) =
                    std::pow(2.0f, static_cast<float>(channel)
                                      / static_cast<float>(internalChannelCount));
                auto &delay = m_feedbackDelays.at(static_cast<std::size_t>(channel));
                delay.resize(maximumFeedbackDelaySamples);
                delay.reset();
            }

            const float sizeSamples = millisecondsToSamples(parameters.sizeMilliseconds);
            const float preDelaySamples = millisecondsToSamples(parameters.preDelayMilliseconds);
            m_sizeTransition.reset(sizeSamples);
            m_preDelayTransition.reset(preDelaySamples);
            m_feedbackGainRamp.reset(feedbackGainFor(
                parameters.sizeMilliseconds, parameters.decaySeconds));
            m_mixRamp.reset(parameters.mixPercent * 0.01f);

            m_activeDampingBank = 0;
            m_transitionDampingBank = 1;
            m_dampingTransitionRemaining = 0;
            m_currentDampingConfiguration = {
                parameters.sizeMilliseconds,
                parameters.decaySeconds,
                parameters.dampingPercent,
            };
            m_requestedDampingConfiguration = m_currentDampingConfiguration;
            m_transitionDampingConfiguration = m_currentDampingConfiguration;
            configureDampingBank(m_dampingBanks.at(0),
                                 m_currentDampingConfiguration, true);
            m_dampingBanks.at(1) = m_dampingBanks.at(0);
            m_configured = true;
        }

        void setParameters(const ParameterSnapshot &parameters) {
            if (!m_configured) {
                return;
            }
            m_sizeTransition.request(millisecondsToSamples(parameters.sizeMilliseconds),
                                     m_transitionSamples);
            m_preDelayTransition.request(
                millisecondsToSamples(parameters.preDelayMilliseconds),
                m_transitionSamples);
            m_feedbackGainRamp.setTarget(
                feedbackGainFor(parameters.sizeMilliseconds,
                                parameters.decaySeconds),
                m_transitionSamples);
            m_mixRamp.setTarget(parameters.mixPercent * 0.01f, m_transitionSamples);
            requestDampingConfiguration({
                parameters.sizeMilliseconds,
                parameters.decaySeconds,
                parameters.dampingPercent,
            });
        }

        void refresh() {
            for (auto &delay : m_preDelayLines) {
                delay.reset();
            }
            for (auto &step : m_diffusionSteps) {
                step.reset();
            }
            for (auto &delay : m_feedbackDelays) {
                delay.reset();
            }
            for (auto &bank : m_dampingBanks) {
                for (auto &filter : bank) {
                    filter.reset();
                }
            }
        }

        void close() {
            refresh();
            m_dampingTransitionRemaining = 0;
            m_configured = false;
        }

        void process(talcs::IAudioSampleContainer &buffer, qint64 startPosition,
                     qint64 length) {
            if (!m_configured || length <= 0) {
                return;
            }
            const int channelCount = std::min(2, buffer.channelCount());
            if (channelCount == 0) {
                return;
            }

            for (qint64 position = startPosition;
                 position < startPosition + length; ++position) {
                StereoArray dryInput;
                dryInput[0] = buffer.sample(0, position);
                dryInput[1] = channelCount > 1
                    ? buffer.sample(1, position)
                    : dryInput[0];

                const float preDelayAlpha = m_preDelayTransition.alpha();
                StereoArray wetInput;
                for (int channel = 0; channel < 2; ++channel) {
                    auto &delay = m_preDelayLines.at(static_cast<std::size_t>(channel));
                    delay.write(dryInput.at(static_cast<std::size_t>(channel)));
                    const float fromValue = delay.read(m_preDelayTransition.from());
                    const float toValue = delay.read(m_preDelayTransition.to());
                    wetInput.at(static_cast<std::size_t>(channel)) =
                        std::lerp(fromValue, toValue, preDelayAlpha);
                }

                InternalArray diffuseInput;
                m_stereoMixer.stereoToMulti(wetInput, diffuseInput);

                const float sizeAlpha = m_sizeTransition.alpha();
                const float sizeSamplesFrom = m_sizeTransition.from();
                const float sizeSamplesTo = m_sizeTransition.to();
                for (auto &step : m_diffusionSteps) {
                    diffuseInput = step.process(diffuseInput,
                                                sizeSamplesFrom,
                                                sizeSamplesTo,
                                                sizeAlpha);
                }

                InternalArray feedbackOutput;
                for (int channel = 0; channel < internalChannelCount; ++channel) {
                    const float ratio = m_feedbackDelayRatios.at(
                        static_cast<std::size_t>(channel));
                    auto &delay = m_feedbackDelays.at(static_cast<std::size_t>(channel));
                    const float fromValue = delay.read(sizeSamplesFrom * ratio);
                    const float toValue = delay.read(sizeSamplesTo * ratio);
                    feedbackOutput.at(static_cast<std::size_t>(channel)) =
                        std::lerp(fromValue, toValue, sizeAlpha);
                }

                InternalArray mixedFeedback = feedbackOutput;
                signalsmith::mix::Householder<float, internalChannelCount>::inPlace(
                    mixedFeedback);
                applyDamping(mixedFeedback);

                const float feedbackGain = m_feedbackGainRamp.next();
                for (int channel = 0; channel < internalChannelCount; ++channel) {
                    const auto index = static_cast<std::size_t>(channel);
                    const float feedbackInput = diffuseInput.at(index)
                        + mixedFeedback.at(index) * feedbackGain;
                    m_feedbackDelays.at(index).write(withoutDenormal(feedbackInput));
                }

                StereoArray wetOutput;
                m_stereoMixer.multiToStereo(feedbackOutput, wetOutput);
                const float wetScale = StereoMixer::scalingFactor2();
                const float mix = m_mixRamp.next();
                const float dry = 1.0f - mix;
                buffer.setSample(0, position,
                                 dryInput[0] * dry + wetOutput[0] * wetScale * mix);
                if (channelCount > 1) {
                    buffer.setSample(1, position,
                                     dryInput[1] * dry + wetOutput[1] * wetScale * mix);
                }

                m_sizeTransition.advance();
                m_preDelayTransition.advance();
                advanceDampingTransition();
            }
        }

    private:
        using DampingBank = std::array<DampingFilter, internalChannelCount>;

        float millisecondsToSamples(float milliseconds) const {
            return milliseconds * 0.001f * static_cast<float>(m_sampleRate);
        }

        static float feedbackGainFor(float sizeMilliseconds,
                                     float decaySeconds) {
            const float typicalLoopSeconds = sizeMilliseconds * 0.0015f;
            const float decibelsPerLoop = -60.0f * typicalLoopSeconds / decaySeconds;
            return std::pow(10.0f, decibelsPerLoop * 0.05f);
        }

        float dampingShelfDb(const DampingConfiguration &configuration) const {
            const float typicalLoopSeconds =
                configuration.sizeMilliseconds * 0.0015f;
            const float decibelsPerLoop = -60.0f * typicalLoopSeconds
                / configuration.decaySeconds;
            return decibelsPerLoop * configuration.dampingPercent * 0.01f;
        }

        void configureDampingBank(DampingBank &bank,
                                  const DampingConfiguration &configuration,
                                  bool resetStates) {
            const double frequency = std::min(
                dampingShelfFrequencyHz, m_sampleRate * 0.45);
            const double scaledFrequency = frequency / m_sampleRate;
            const float shelfDb = dampingShelfDb(configuration);
            for (auto &filter : bank) {
                if (resetStates) {
                    filter.reset();
                }
                filter.highShelfDb(scaledFrequency, shelfDb, 1.0,
                                   signalsmith::filters::BiquadDesign::oneSided);
            }
        }

        void requestDampingConfiguration(
            const DampingConfiguration &configuration) {
            if (configurationsEqual(configuration,
                                    m_requestedDampingConfiguration)) {
                return;
            }
            m_requestedDampingConfiguration = configuration;
            if (m_dampingTransitionRemaining == 0) {
                startDampingTransition();
            }
        }

        void startDampingTransition() {
            if (configurationsEqual(m_currentDampingConfiguration,
                                    m_requestedDampingConfiguration)) {
                return;
            }
            m_transitionDampingBank = 1 - m_activeDampingBank;
            m_dampingBanks.at(static_cast<std::size_t>(m_transitionDampingBank)) =
                m_dampingBanks.at(static_cast<std::size_t>(m_activeDampingBank));
            configureDampingBank(
                m_dampingBanks.at(static_cast<std::size_t>(m_transitionDampingBank)),
                m_requestedDampingConfiguration, false);
            m_transitionDampingConfiguration = m_requestedDampingConfiguration;
            m_dampingTransitionRemaining = m_transitionSamples;
        }

        void applyDamping(InternalArray &values) {
            auto &activeBank = m_dampingBanks.at(
                static_cast<std::size_t>(m_activeDampingBank));
            if (m_dampingTransitionRemaining == 0) {
                for (int channel = 0; channel < internalChannelCount; ++channel) {
                    const auto index = static_cast<std::size_t>(channel);
                    values.at(index) = withoutDenormal(
                        activeBank.at(index)(values.at(index)));
                }
                return;
            }

            auto &transitionBank = m_dampingBanks.at(
                static_cast<std::size_t>(m_transitionDampingBank));
            const float alpha = 1.0f
                - static_cast<float>(m_dampingTransitionRemaining)
                    / static_cast<float>(m_transitionSamples);
            for (int channel = 0; channel < internalChannelCount; ++channel) {
                const auto index = static_cast<std::size_t>(channel);
                const float input = values.at(index);
                const float activeValue = activeBank.at(index)(input);
                const float transitionValue = transitionBank.at(index)(input);
                values.at(index) = withoutDenormal(
                    std::lerp(activeValue, transitionValue, alpha));
            }
        }

        void advanceDampingTransition() {
            if (m_dampingTransitionRemaining == 0) {
                return;
            }
            --m_dampingTransitionRemaining;
            if (m_dampingTransitionRemaining > 0) {
                return;
            }
            m_activeDampingBank = m_transitionDampingBank;
            m_currentDampingConfiguration = m_transitionDampingConfiguration;
            if (!configurationsEqual(m_currentDampingConfiguration,
                                     m_requestedDampingConfiguration)) {
                startDampingTransition();
            }
        }

        std::array<Delay, 2> m_preDelayLines;
        std::array<DiffusionStep, diffusionStepCount> m_diffusionSteps;
        std::array<Delay, internalChannelCount> m_feedbackDelays;
        std::array<float, internalChannelCount> m_feedbackDelayRatios{};
        std::array<DampingBank, 2> m_dampingBanks;
        StereoMixer m_stereoMixer;
        TapTransition m_sizeTransition;
        TapTransition m_preDelayTransition;
        LinearRamp m_feedbackGainRamp;
        LinearRamp m_mixRamp;
        DampingConfiguration m_currentDampingConfiguration;
        DampingConfiguration m_requestedDampingConfiguration;
        DampingConfiguration m_transitionDampingConfiguration;
        double m_sampleRate{44100.0};
        int m_transitionSamples{1};
        int m_activeDampingBank{};
        int m_transitionDampingBank{1};
        int m_dampingTransitionRemaining{};
        bool m_configured{};
    };

    ReverbProcessor::ReverbProcessor()
        : m_engine(std::make_unique<ReverbEngine>()) {
        setParameters(defaultSizeMilliseconds, defaultDecaySeconds,
                      defaultDampingPercent, defaultPreDelayMilliseconds,
                      defaultMixPercent);
    }

    ReverbProcessor::~ReverbProcessor() = default;

    void ReverbProcessor::setParameters(double sizeMilliseconds,
                                        double decaySeconds,
                                        double dampingPercent,
                                        double preDelayMilliseconds,
                                        double mixPercent) {
        const float normalizedSize = normalizedParameter(
            sizeMilliseconds, defaultSizeMilliseconds,
            minimumSizeMilliseconds, maximumSizeMilliseconds);
        const float normalizedDecay = normalizedParameter(
            decaySeconds, defaultDecaySeconds,
            minimumDecaySeconds, maximumDecaySeconds);
        const float normalizedDamping = normalizedParameter(
            dampingPercent, defaultDampingPercent,
            minimumDampingPercent, maximumDampingPercent);
        const float normalizedPreDelay = normalizedParameter(
            preDelayMilliseconds, defaultPreDelayMilliseconds,
            minimumPreDelayMilliseconds, maximumPreDelayMilliseconds);
        const float normalizedMix = normalizedParameter(
            mixPercent, defaultMixPercent,
            minimumMixPercent, maximumMixPercent);

        m_parameterRevision.fetch_add(1, std::memory_order_acq_rel);
        m_sizeMillisecondsBits.store(floatBits(normalizedSize),
                                     std::memory_order_relaxed);
        m_decaySecondsBits.store(floatBits(normalizedDecay),
                                 std::memory_order_relaxed);
        m_dampingPercentBits.store(floatBits(normalizedDamping),
                                   std::memory_order_relaxed);
        m_preDelayMillisecondsBits.store(floatBits(normalizedPreDelay),
                                         std::memory_order_relaxed);
        m_mixPercentBits.store(floatBits(normalizedMix),
                               std::memory_order_relaxed);
        m_parameterRevision.fetch_add(1, std::memory_order_release);
    }

    void ReverbProcessor::refresh() {
        m_refreshRequested.store(true, std::memory_order_release);
    }

    bool ReverbProcessor::open(qint64 bufferSize, double sampleRate) {
        if (!AudioSource::open(bufferSize, sampleRate)) {
            return false;
        }

        ParameterSnapshot snapshot{
            static_cast<float>(defaultSizeMilliseconds),
            static_cast<float>(defaultDecaySeconds),
            static_cast<float>(defaultDampingPercent),
            static_cast<float>(defaultPreDelayMilliseconds),
            static_cast<float>(defaultMixPercent),
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

    void ReverbProcessor::close() {
        m_engine->close();
        m_appliedRevision = 0;
        m_refreshRequested.store(false, std::memory_order_relaxed);
        AudioSource::close();
    }

    qint64 ReverbProcessor::processReading(
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
        m_engine->process(*readData.buffer, readData.startPos, readData.length);
        return readData.length;
    }

    bool ReverbProcessor::tryReadParameterSnapshot(
        ParameterSnapshot &snapshot, std::uint64_t &revision) const {
        const auto firstRevision = m_parameterRevision.load(std::memory_order_acquire);
        if (firstRevision % 2 != 0) {
            return false;
        }

        snapshot.sizeMilliseconds = floatFromBits(
            m_sizeMillisecondsBits.load(std::memory_order_relaxed));
        snapshot.decaySeconds = floatFromBits(
            m_decaySecondsBits.load(std::memory_order_relaxed));
        snapshot.dampingPercent = floatFromBits(
            m_dampingPercentBits.load(std::memory_order_relaxed));
        snapshot.preDelayMilliseconds = floatFromBits(
            m_preDelayMillisecondsBits.load(std::memory_order_relaxed));
        snapshot.mixPercent = floatFromBits(
            m_mixPercentBits.load(std::memory_order_relaxed));

        const auto secondRevision = m_parameterRevision.load(std::memory_order_acquire);
        if (firstRevision != secondRevision || secondRevision % 2 != 0) {
            return false;
        }
        revision = secondRevision;
        return true;
    }

}
