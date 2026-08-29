// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PARAMETERINFOPROVIDER_P_H
#define DIFFSCOPE_COREPLUGIN_PARAMETERINFOPROVIDER_P_H

#include <coreplugin/ParameterInfoProvider.h>

#include <QMetaObject>
#include <QPointer>
#include <QVector>

namespace Core {

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

#endif // DIFFSCOPE_COREPLUGIN_PARAMETERINFOPROVIDER_P_H
