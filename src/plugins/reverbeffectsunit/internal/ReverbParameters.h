// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBPARAMETERS_H
#define DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBPARAMETERS_H

namespace ReverbEffectsUnit::Internal {

    inline constexpr double minimumSizeMilliseconds = 10.0;
    inline constexpr double maximumSizeMilliseconds = 120.0;
    inline constexpr double defaultSizeMilliseconds = 40.0;

    inline constexpr double minimumDecaySeconds = 0.02;
    inline constexpr double maximumDecaySeconds = 6.0;
    inline constexpr double defaultDecaySeconds = 0.8;

    inline constexpr double minimumDampingPercent = 0.0;
    inline constexpr double maximumDampingPercent = 100.0;
    inline constexpr double defaultDampingPercent = 50.0;

    inline constexpr double minimumPreDelayMilliseconds = 0.0;
    inline constexpr double maximumPreDelayMilliseconds = 250.0;
    inline constexpr double defaultPreDelayMilliseconds = 20.0;

    inline constexpr double minimumMixPercent = 0.0;
    inline constexpr double maximumMixPercent = 100.0;
    inline constexpr double defaultMixPercent = 25.0;

}

#endif // DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBPARAMETERS_H
