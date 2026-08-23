// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EffectsUnitCollection.h"

#include <effectsunitmanager/EffectsUnitClass.h>
#include <effectsunitmanager/private/EffectsUnitCollection_p.h>

namespace EffectsUnitManager {

    static EffectsUnitCollection *s_instance{};

    EffectsUnitCollection::EffectsUnitCollection(QObject *parent)
        : QObject(parent), d_ptr(new EffectsUnitCollectionPrivate) {
        Q_ASSERT(!s_instance);
        s_instance = this;
        Q_D(EffectsUnitCollection);
        d->q_ptr = this;
    }

    EffectsUnitCollection::~EffectsUnitCollection() {
        Q_ASSERT(s_instance == this);
        s_instance = nullptr;
    }

    EffectsUnitCollection *EffectsUnitCollection::instance() {
        return s_instance;
    }

    EffectsUnitCollection *EffectsUnitCollection::create(QQmlEngine *, QJSEngine *) {
        return instance();
    }

    bool EffectsUnitCollection::registerEffectsUnitClass(const QString &id, EffectsUnitClass *effectsUnitClass) {
        Q_D(EffectsUnitCollection);
        Q_ASSERT(effectsUnitClass);
        if (id.isEmpty() || !effectsUnitClass || d->classes.contains(id)) {
            return false;
        }
        d->classes.insert(id, effectsUnitClass);
        d->ids.append(id);
        Q_EMIT effectsUnitClassRegistered(id, effectsUnitClass);
        Q_EMIT effectsUnitIdsChanged();
        return true;
    }

    EffectsUnitClass *EffectsUnitCollection::effectsUnitClass(const QString &id) const {
        Q_D(const EffectsUnitCollection);
        return d->classes.value(id);
    }

    QStringList EffectsUnitCollection::effectsUnitIds() const {
        Q_D(const EffectsUnitCollection);
        return d->ids;
    }

}

#include "moc_EffectsUnitCollection.cpp"
