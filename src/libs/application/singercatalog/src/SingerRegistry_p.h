// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_SINGERREGISTRY_P_H
#define DIFFSCOPE_SINGERCATALOG_SINGERREGISTRY_P_H

#include <singercatalog/SingerRegistry.h>

#include <QMap>

namespace SingerCatalog {

    class SingerRegistryPrivate {
        Q_DECLARE_PUBLIC(SingerRegistry)

    public:
        struct ArchitectureEntry {
            ArchitectureInfo info;
            QMap<QString, SingerInfo> singers;
        };

        SingerRegistry *q_ptr{};
        QMap<QString, ArchitectureEntry> architectures;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_SINGERREGISTRY_P_H
