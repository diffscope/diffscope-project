#include "WaveformSingerSynthesizer.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Audio::Internal {

    namespace {

        constexpr double twoPi = 6.283185307179586476925286766559;
        constexpr double pianoGain = 0.09;
        constexpr double sineGain = 0.089;
        constexpr double choirGain = 0.12;
        constexpr double pianoVelocity = 0.85;

        struct Harmonic {
            int number;
            double amplitude;
        };

        constexpr std::array pianoHarmonics{
            Harmonic{1, 1.00}, Harmonic{2, 0.48}, Harmonic{3, 0.24},
            Harmonic{4, 0.12}, Harmonic{5, 0.05}, Harmonic{6, 0.07},
        };
        constexpr std::array choirHarmonics{
            Harmonic{1, 0.75}, Harmonic{2, 1.00}, Harmonic{3, 0.65}, Harmonic{4, 0.42},
            Harmonic{5, 0.26}, Harmonic{6, 0.18}, Harmonic{7, 0.11}, Harmonic{8, 0.07},
        };
        constexpr std::array choirVoices{
            std::array{-4.0, 0.28}, std::array{0.0, 0.44}, std::array{4.0, 0.28},
        };

    }

    float WaveformSingerSynthesizer::render(
        double baseCycles, double frequency, double noteTime, double noteOffTime,
        std::uint64_t seed, const std::array<double, waveformSingerTypeCount> &weights,
        double sampleRate) {
        const auto piano = renderPiano(baseCycles, frequency, noteTime, noteOffTime, sampleRate);
        const auto sine = renderSine(baseCycles, noteTime, noteOffTime);
        const auto choir = renderChoir(baseCycles, frequency, noteTime, noteOffTime, seed, sampleRate);
        return static_cast<float>(
            weights[0] * pianoGain * piano +
            weights[1] * sineGain * sine +
            weights[2] * choirGain * choir);
    }

    double WaveformSingerSynthesizer::envelope(
        double time, double noteOff, double attack, double decay, double sustain, double release) {
        attack = std::max(attack, 1.0e-9);
        decay = std::max(decay, 1.0e-9);
        release = std::max(release, 1.0e-9);
        const double attackEnd = attack;
        const double decayEnd = attack + decay;

        const auto levelBeforeRelease = [=](double t) {
            if (t < attackEnd) {
                return std::max(0.0, t / attack);
            }
            if (t < decayEnd) {
                return 1.0 - (1.0 - sustain) * ((t - attackEnd) / decay);
            }
            return sustain;
        };
        if (time < noteOff) {
            return levelBeforeRelease(time);
        }
        return levelBeforeRelease(noteOff) * std::max(0.0, 1.0 - (time - noteOff) / release);
    }

    double WaveformSingerSynthesizer::phaseOffset(std::uint64_t seed, int voice, int harmonic) {
        auto value = seed ^ (static_cast<std::uint64_t>(voice + 1) * 0x9e3779b97f4a7c15ULL);
        value ^= static_cast<std::uint64_t>(harmonic + 1) * 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 30;
        value *= 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 27;
        value *= 0x94d049bb133111ebULL;
        value ^= value >> 31;
        return twoPi * static_cast<double>(value >> 11) / static_cast<double>(std::uint64_t{1} << 53);
    }

    double WaveformSingerSynthesizer::renderPiano(
        double baseCycles, double frequency, double noteTime, double noteOffTime, double sampleRate) {
        double sample = 0.0;
        for (const auto &harmonic : pianoHarmonics) {
            if (frequency * harmonic.number >= sampleRate * 0.45) {
                continue;
            }
            const auto h = static_cast<double>(harmonic.number);
            const auto attack = 0.006 / h;
            const auto decay = 2.4 / std::pow(h, 1.25);
            const auto sustain = harmonic.number == 1 ? 0.04 : harmonic.number == 2 ? 0.015 : 0.0;
            const auto release = 0.22 / std::sqrt(h);
            const auto velocityGain = harmonic.number <= 2
                ? 0.35 + 0.65 * pianoVelocity
                : pianoVelocity * pianoVelocity;
            sample += harmonic.amplitude * velocityGain *
                      envelope(noteTime, noteOffTime, attack, decay, sustain, release) *
                      std::sin(twoPi * baseCycles * h);
        }
        return sample;
    }

    double WaveformSingerSynthesizer::renderSine(double baseCycles, double noteTime, double noteOffTime) {
        return envelope(noteTime, noteOffTime, 0.010, 1.0e-9, 1.0, 0.120) *
               std::sin(twoPi * baseCycles);
    }

    double WaveformSingerSynthesizer::renderChoir(
        double baseCycles, double frequency, double noteTime, double noteOffTime,
        std::uint64_t seed, double sampleRate) {
        double sample = 0.0;
        for (int voiceIndex = 0; voiceIndex < static_cast<int>(choirVoices.size()); ++voiceIndex) {
            const auto detuneRatio = std::pow(2.0, choirVoices[voiceIndex][0] / 1200.0);
            const auto voiceGain = choirVoices[voiceIndex][1];
            for (const auto &harmonic : choirHarmonics) {
                if (frequency * detuneRatio * harmonic.number >= sampleRate * 0.45) {
                    continue;
                }
                const auto h = static_cast<double>(harmonic.number);
                const auto attack = 0.18 + 0.02 * (h - 1.0);
                const auto sustain = std::max(0.50, 0.94 - 0.06 * (h - 1.0));
                const auto release = 0.55 - 0.03 * (h - 1.0);
                sample += voiceGain * harmonic.amplitude *
                          envelope(noteTime, noteOffTime, attack, 0.25, sustain, release) *
                          std::sin(twoPi * baseCycles * detuneRatio * h +
                                   phaseOffset(seed, voiceIndex, harmonic.number));
            }
        }
        return sample;
    }

}
