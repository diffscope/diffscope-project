// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERPARAMETERS_H
#define DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERPARAMETERS_H

#include <QList>

namespace EqualizerEffectsUnit::Internal {

    inline constexpr double minimumFrequencyHz = 20.0;
    inline constexpr double maximumFrequencyHz = 20000.0;
    inline constexpr double minimumGainDb = -24.0;
    inline constexpr double maximumGainDb = 24.0;
    inline constexpr double minimumQ = 0.1;
    inline constexpr double maximumQ = 24.0;
    inline constexpr double defaultQ = 1.0;
    inline constexpr int maximumBandCount = 16;
    inline constexpr int equalizerResponsePointCount = 256;
    inline constexpr int equalizerSpectrumBinCount = 192;

    inline constexpr double initialLowFrequencyHz = 112.46826503806983;
    inline constexpr double initialMidFrequencyHz = 632.4555320336759;
    inline constexpr double initialHighFrequencyHz = 3556.5588200778457;

    enum class EqualizerBandType {
        Bell,
        LowShelf,
        HighShelf,
    };

    struct EqualizerBand {
        EqualizerBandType type{EqualizerBandType::Bell};
        double frequencyHz{1000.0};
        double gainDb{};
        double q{defaultQ};
        bool enabled{true};
        bool solo{};
    };

    using EqualizerBandList = QList<EqualizerBand>;

    inline EqualizerBandList defaultEqualizerBands() {
        return {
            {EqualizerBandType::Bell, initialLowFrequencyHz, 0.0, defaultQ},
            {EqualizerBandType::Bell, initialMidFrequencyHz, 0.0, defaultQ},
            {EqualizerBandType::Bell, initialHighFrequencyHz, 0.0, defaultQ},
        };
    }

}

#endif // DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERPARAMETERS_H
