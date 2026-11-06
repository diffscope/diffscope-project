// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFOPROVIDER_P_H
#define DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFOPROVIDER_P_H

#include <singercatalog/ArchitectureInfoProvider.h>

#include <QMetaObject>
#include <QPointer>
#include <QVector>

#include <singercatalog/SingerRegistry.h>

namespace SingerCatalog {

    class ArchitectureInfoProviderPrivate {
        Q_DECLARE_PUBLIC(ArchitectureInfoProvider)

    public:
        void update();
        void disconnectRegistry();

        ArchitectureInfoProvider *q_ptr{};
        QPointer<SingerRegistry> registry;
        QString architectureId;
        ArchitectureInfo info;
        bool exists{};
        QStringList singerIds;
        QVector<QMetaObject::Connection> registryConnections;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFOPROVIDER_P_H
