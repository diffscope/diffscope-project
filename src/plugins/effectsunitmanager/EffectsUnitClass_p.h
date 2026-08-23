// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITCLASS_P_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITCLASS_P_H

#include <effectsunitmanager/EffectsUnitClass.h>

namespace EffectsUnitManager {

    class EffectsUnitClassPrivate {
        Q_DECLARE_PUBLIC(EffectsUnitClass)

    public:
        EffectsUnitClass *q_ptr{};
        QString name;
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITCLASS_P_H
