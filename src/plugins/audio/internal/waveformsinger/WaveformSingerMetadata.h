#ifndef DIFFSCOPE_AUDIO_WAVEFORMSINGERMETADATA_H
#define DIFFSCOPE_AUDIO_WAVEFORMSINGERMETADATA_H

#include <QCoreApplication>
#include <QList>
#include <QString>

#include <coreplugin/ArchitectureInfo.h>
#include <coreplugin/SingerInfo.h>

namespace Core {
    class SingerRegistry;
}

namespace Audio::Internal {

    class WaveformSingerMetadata {
        Q_DECLARE_TR_FUNCTIONS(Audio::Internal::WaveformSingerMetadata)

    public:
        struct SingerEntry {
            QString id;
            Core::SingerInfo info;
        };

        static QString architectureId();
        static Core::ArchitectureInfo architectureInfo();
        static QList<SingerEntry> singers();

        static bool registerAll(Core::SingerRegistry *registry);
    };

}

#endif // DIFFSCOPE_AUDIO_WAVEFORMSINGERMETADATA_H
