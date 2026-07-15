#include "WaveformSingerMetadata.h"

#include <coreplugin/SingerRegistry.h>

namespace Audio::Internal {

    QString WaveformSingerMetadata::architectureId() {
        return QStringLiteral("org.diffscope.waveform");
    }

    Core::ArchitectureInfo WaveformSingerMetadata::architectureInfo() {
        Core::ArchitectureInfo info;
        info.setName(tr("Waveform"));

        Core::ArchitectureInfo::Parameter directParameter;
        Core::ArchitectureInfo::ParameterMap parameters;
        parameters.insert(QStringLiteral("pitch"), directParameter);
        parameters.insert(QStringLiteral("energy"), directParameter);
        info.setParameters(parameters);
        return info;
    }

    QList<WaveformSingerMetadata::SingerEntry> WaveformSingerMetadata::singers() {
        const auto createSinger = [](const QString &name, const QUrl &avatar) {
            Core::SingerInfo info;
            info.setName(name);
            info.setMixGroup(QStringLiteral("waveform"));
            info.setAvatarUrl(avatar);
            return info;
        };

        return {
            {QStringLiteral("piano"), createSinger(tr("Electric Piano"), QUrl("qrc:/diffscope/audio/singeravatar/piano.png"))},
            {QStringLiteral("sinewave"), createSinger(tr("Sine Wave"), QUrl("qrc:/diffscope/audio/singeravatar/sinewave.png"))},
            {QStringLiteral("choir"), createSinger(tr("Choir"), QUrl("qrc:/diffscope/audio/singeravatar/choir.png"))},
        };
    }

    bool WaveformSingerMetadata::registerAll(Core::SingerRegistry *registry) {
        if (!registry)
            return false;

        const auto id = architectureId();
        if (!registry->registerArchitecture(id, architectureInfo()))
            return false;

        bool success = true;
        for (const auto &singer : singers()) {
            const bool singerRegistered = registry->registerSinger(id, singer.id, singer.info);
            success = singerRegistered && success;
        }
        return success;
    }

}
