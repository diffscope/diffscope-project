#ifndef DIFFSCOPE_COREPLUGIN_SINGERREGISTRY_P_H
#define DIFFSCOPE_COREPLUGIN_SINGERREGISTRY_P_H

#include <coreplugin/SingerRegistry.h>

#include <QMap>

namespace Core {

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

#endif // DIFFSCOPE_COREPLUGIN_SINGERREGISTRY_P_H
