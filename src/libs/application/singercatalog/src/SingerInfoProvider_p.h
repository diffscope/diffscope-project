// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_SINGERINFOPROVIDER_P_H
#define DIFFSCOPE_SINGERCATALOG_SINGERINFOPROVIDER_P_H

#include <singercatalog/SingerInfoProvider.h>

#include <QMetaObject>
#include <QPointer>
#include <QVector>

#include <singercatalog/SingerRegistry.h>

namespace SingerCatalog {

    class SingerInfoProviderPrivate {
        Q_DECLARE_PUBLIC(SingerInfoProvider)

    public:
        void update();
        void disconnectRegistry();

        SingerInfoProvider *q_ptr{};
        QPointer<SingerRegistry> registry;
        QString architectureId;
        QString singerId;
        SingerInfo info;
        bool exists{};
        QVector<QMetaObject::Connection> registryConnections;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_SINGERINFOPROVIDER_P_H
