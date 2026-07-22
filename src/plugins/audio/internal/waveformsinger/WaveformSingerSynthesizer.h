#ifndef DIFFSCOPE_AUDIO_WAVEFORMSINGERSYNTHESIZER_H
#define DIFFSCOPE_AUDIO_WAVEFORMSINGERSYNTHESIZER_H

#include <array>
#include <cstdint>

#include <audio/internal/WaveformSingerModel.h>

namespace Audio::Internal {

    class WaveformSingerSynthesizer {
    public:
        static constexpr double maximumReleaseSeconds() {
            return 0.55;
        }

        static float render(double baseCycles, double frequency, double noteTime,
                            double noteOffTime, std::uint64_t seed,
                            const std::array<double, waveformSingerTypeCount> &weights,
                            double sampleRate);

    private:
        static double envelope(double time, double noteOff, double attack,
                               double decay, double sustain, double release);
        static double phaseOffset(std::uint64_t seed, int voice, int harmonic);
        static double renderPiano(double baseCycles, double frequency, double noteTime,
                                  double noteOffTime, double sampleRate);
        static double renderSine(double baseCycles, double noteTime, double noteOffTime);
        static double renderChoir(double baseCycles, double frequency, double noteTime,
                                  double noteOffTime, std::uint64_t seed, double sampleRate);
    };

}

#endif // DIFFSCOPE_AUDIO_WAVEFORMSINGERSYNTHESIZER_H
