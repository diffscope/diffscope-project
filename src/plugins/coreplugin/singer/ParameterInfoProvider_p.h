#ifndef DIFFSCOPE_COREPLUGIN_PARAMETERINFOPROVIDER_P_H
#define DIFFSCOPE_COREPLUGIN_PARAMETERINFOPROVIDER_P_H

#include <QMetaObject>
#include <QPointer>
#include <QVector>

#include <coreplugin/ParameterInfoProvider.h>

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
