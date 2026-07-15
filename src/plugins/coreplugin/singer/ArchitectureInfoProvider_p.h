#ifndef DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFOPROVIDER_P_H
#define DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFOPROVIDER_P_H

#include <coreplugin/ArchitectureInfoProvider.h>

#include <QMetaObject>
#include <QPointer>
#include <QVector>

#include <coreplugin/SingerRegistry.h>

namespace Core {

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

#endif // DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFOPROVIDER_P_H
