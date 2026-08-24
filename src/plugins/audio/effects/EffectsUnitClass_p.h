// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EFFECTSUNITCLASS_P_H
#define DIFFSCOPE_AUDIO_EFFECTSUNITCLASS_P_H

#include <audio/EffectsUnitClass.h>

namespace Audio {

    class EffectsUnitClassPrivate {
        Q_DECLARE_PUBLIC(EffectsUnitClass)

    public:
        EffectsUnitClass *q_ptr{};
        QString name;
    };

}

#endif // DIFFSCOPE_AUDIO_EFFECTSUNITCLASS_P_H
