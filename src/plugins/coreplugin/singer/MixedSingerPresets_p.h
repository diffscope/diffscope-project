// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_MIXEDSINGERPRESETS_P_H
#define DIFFSCOPE_COREPLUGIN_MIXEDSINGERPRESETS_P_H

#include <coreplugin/MixedSingerPresets.h>

#include <QList>

namespace Core {

    class MixedSingerPresetsPrivate {
        Q_DECLARE_PUBLIC(MixedSingerPresets)

    public:
        struct Preset {
            QString id;
            QString name;
            SourcesPickerModel *sources{};
        };

        MixedSingerPresets *q_ptr{};
        QList<Preset> presets;

        void load();
        void save() const;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_MIXEDSINGERPRESETS_P_H
