// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EFFECTSUNITCOLLECTION_P_H
#define DIFFSCOPE_AUDIO_EFFECTSUNITCOLLECTION_P_H

#include <QHash>

#include <audio/EffectsUnitCollection.h>

namespace Audio {

    class EffectsUnitCollectionPrivate {
        Q_DECLARE_PUBLIC(EffectsUnitCollection)

    public:
        EffectsUnitCollection *q_ptr{};
        QHash<QString, EffectsUnitClass *> classes;
        QStringList ids;
    };

}

#endif // DIFFSCOPE_AUDIO_EFFECTSUNITCOLLECTION_P_H
