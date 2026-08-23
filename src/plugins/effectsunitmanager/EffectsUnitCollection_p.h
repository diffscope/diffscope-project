// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITCOLLECTION_P_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITCOLLECTION_P_H

#include <QHash>

#include <effectsunitmanager/EffectsUnitCollection.h>

namespace EffectsUnitManager {

    class EffectsUnitCollectionPrivate {
        Q_DECLARE_PUBLIC(EffectsUnitCollection)

    public:
        EffectsUnitCollection *q_ptr{};
        QHash<QString, EffectsUnitClass *> classes;
        QStringList ids;
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSUNITCOLLECTION_P_H
