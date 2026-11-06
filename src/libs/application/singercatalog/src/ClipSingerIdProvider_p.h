// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_CLIPSINGERIDPROVIDER_P_H
#define DIFFSCOPE_SINGERCATALOG_CLIPSINGERIDPROVIDER_P_H

#include <singercatalog/ClipSingerIdProvider.h>

#include <QMetaObject>
#include <QPointer>
#include <QVector>

#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/Sources.h>

namespace SingerCatalog {

    class ClipSingerIdProviderPrivate {
        Q_DECLARE_PUBLIC(ClipSingerIdProvider)

    public:
        void disconnectSources();
        void disconnectSingerTree();
        void rebuildSingerTreeBindings();
        void bindSingerList(dspx::SingerList *singerList);
        void updateArchitectureId();
        void updateSingerTree();

        QVariant singerToVariant(dspx::Singer *singer) const;
        QVariantList singerListToVariantList(dspx::SingerList *singerList) const;

        ClipSingerIdProvider *q_ptr{};
        QPointer<dspx::Sources> sources;
        QString architectureId;
        QVariantList singerTree;
        QVector<QMetaObject::Connection> sourceConnections;
        QVector<QMetaObject::Connection> singerTreeConnections;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_CLIPSINGERIDPROVIDER_P_H
