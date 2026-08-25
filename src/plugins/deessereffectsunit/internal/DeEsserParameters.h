// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSERPARAMETERS_H
#define DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSERPARAMETERS_H

namespace DeEsserEffectsUnit::Internal {

    inline constexpr double minimumFrequencyHz = 100.0;
    inline constexpr double maximumFrequencyHz = 12000.0;
    inline constexpr double defaultFrequencyHz = 6000.0;

    inline constexpr double minimumBandwidthHz = 100.0;
    inline constexpr double maximumBandwidthHz = 6000.0;
    inline constexpr double defaultBandwidthHz = 3000.0;

    inline constexpr double minimumThresholdDb = -96.0;
    inline constexpr double maximumThresholdDb = 0.0;
    inline constexpr double defaultThresholdDb = -30.0;

    inline constexpr bool defaultOutputSibilanceOnly = false;

    inline constexpr double compressorAttackMilliseconds = 2.0;
    inline constexpr double compressorReleaseMilliseconds = 50.0;
    inline constexpr double maximumCompressionDb = -12.0;

    inline constexpr float meterFloorDb = -96.0f;
    inline constexpr int deEsserSpectrumBinCount = 192;

}

#endif // DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSERPARAMETERS_H
