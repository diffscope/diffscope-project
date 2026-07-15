#ifndef DIFFSCOPE_COREPLUGIN_SINGERINFOPROVIDER_P_H
#define DIFFSCOPE_COREPLUGIN_SINGERINFOPROVIDER_P_H

#include <coreplugin/SingerInfoProvider.h>

#include <QMetaObject>
#include <QPointer>
#include <QVector>

#include <coreplugin/SingerRegistry.h>

namespace Core {

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

#endif // DIFFSCOPE_COREPLUGIN_SINGERINFOPROVIDER_P_H
