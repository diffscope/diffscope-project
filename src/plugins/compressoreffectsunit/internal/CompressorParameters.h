// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSORPARAMETERS_H
#define DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSORPARAMETERS_H

namespace CompressorEffectsUnit::Internal {

    inline constexpr double minimumThresholdDb = -96.0;
    inline constexpr double maximumThresholdDb = 0.0;
    inline constexpr double defaultThresholdDb = -12.0;

    inline constexpr double minimumRatio = 1.0;
    inline constexpr double maximumRatio = 100.0;
    inline constexpr double defaultRatio = 4.0;

    inline constexpr double minimumAttackMilliseconds = 0.01;
    inline constexpr double maximumAttackMilliseconds = 1000.0;
    inline constexpr double defaultAttackMilliseconds = 10.0;

    inline constexpr double minimumReleaseMilliseconds = 10.0;
    inline constexpr double maximumReleaseMilliseconds = 10000.0;
    inline constexpr double defaultReleaseMilliseconds = 100.0;

    inline constexpr float meterFloorDb = -96.0f;

}

#endif // DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSORPARAMETERS_H
