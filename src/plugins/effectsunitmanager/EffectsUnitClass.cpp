// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EffectsUnitClass.h"

#include <effectsunitmanager/private/EffectsUnitClass_p.h>

namespace EffectsUnitManager {

    EffectsUnitClass::EffectsUnitClass(const QString &name, QObject *parent)
        : QObject(parent), d_ptr(new EffectsUnitClassPrivate) {
        Q_D(EffectsUnitClass);
        d->q_ptr = this;
        d->name = name;
    }

    EffectsUnitClass::~EffectsUnitClass() = default;

    QString EffectsUnitClass::name() const {
        Q_D(const EffectsUnitClass);
        return d->name;
    }

}

#include "moc_EffectsUnitClass.cpp"
