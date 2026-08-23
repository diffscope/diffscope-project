// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNIT_P_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNIT_P_H

#include <memory>

#include <effectsunitmanager/EffectsUnit.h>

namespace EffectsUnitManager {

    class EffectsUnitPrivate {
        Q_DECLARE_PUBLIC(EffectsUnit)

    public:
        EffectsUnit *q_ptr{};
        QQuickItem *editor{};
        std::unique_ptr<talcs::AudioSource> processor;
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNIT_P_H
