// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_PARAMETERINFOPROVIDER_P_H
#define DIFFSCOPE_SINGERCATALOG_PARAMETERINFOPROVIDER_P_H

#include <singercatalog/ParameterInfoProvider.h>

#include <QMetaObject>
#include <QPointer>
#include <QVector>

namespace SingerCatalog {

    class ParameterInfoProviderPrivate {
        Q_DECLARE_PUBLIC(ParameterInfoProvider)

    public:
        void update();
        void disconnectRegistry();

        ParameterInfoProvider *q_ptr{};
        QPointer<SingerRegistry> registry;
        QString architectureId;
        QString parameterId;
        bool transform{};
        ParameterInfo info;
        bool exists{};
        QVector<QMetaObject::Connection> registryConnections;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_PARAMETERINFOPROVIDER_P_H
