// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EFFECTSUNIT_P_H
#define DIFFSCOPE_AUDIO_EFFECTSUNIT_P_H

#include <audio/EffectsUnit.h>

#include <memory>

namespace Audio {

    class EffectsUnitPrivate {
        Q_DECLARE_PUBLIC(EffectsUnit)

    public:
        EffectsUnit *q_ptr{};
        QQuickItem *editor{};
        std::unique_ptr<talcs::AudioSource> processor;
    };

}

#endif // DIFFSCOPE_AUDIO_EFFECTSUNIT_P_H
