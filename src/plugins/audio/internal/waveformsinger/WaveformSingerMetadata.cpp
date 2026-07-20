#include "WaveformSingerMetadata.h"

#include <coreplugin/SingerRegistry.h>

namespace Audio::Internal {

    QString WaveformSingerMetadata::architectureId() {
        return QStringLiteral("org.diffscope.waveform");
    }

    Core::ArchitectureInfo WaveformSingerMetadata::architectureInfo() {
        Core::ArchitectureInfo info;
        info.setName(tr("Waveform"));

        Core::ArchitectureInfo::ParameterMap parameters;
        parameters.insert(QStringLiteral("energy"), {
            .displayName = tr("Energy"),
            .bottomValue = -96000,
            .topValue = 0,
            .defaultValue = 0,
            .fillMode = Core::ParameterInfo::FillMode::BottomFill,
            .valueType = Core::ParameterInfo::ValueType::Relative,
            .divisionValue = 12000,
            .showDefaultValue = false,
            .showDivision = true,
            .normalize = [](const Core::ParameterInfo &self, int value) {
                return 1.0 - std::pow(1.0 * (self.topValue - value) / (self.topValue - self.bottomValue), 1.25);
            },
            .denormalize = [](const Core::ParameterInfo &self, double value) {
                return static_cast<int>(self.topValue - std::pow(1.0 - value, 0.8) * (self.topValue - self.bottomValue));
            },
            .toDisplayValue = [](const Core::ParameterInfo &, int value) {
                return value / 1000.0;
            },
            .fromDisplayValue = [](const Core::ParameterInfo &, double value) {
                return static_cast<int>(value * 1000.0);
            },
            .toDisplayString = [](const Core::ParameterInfo &, int value) {
                return QLocale().toString(value / 1000.0, 'f', 3) + tr(" dB");
            }
        });
        parameters.insert(QStringLiteral("tone_shift"), {
            .displayName = tr("Tone shift"),
            .bottomValue = -1200,
            .topValue = 1200,
            .defaultValue = 0,
            .fillMode = Core::ParameterInfo::FillMode::BaselineFill,
            .valueType = Core::ParameterInfo::ValueType::Relative,
            .divisionValue = 0,
            .showDefaultValue = true,
            .showDivision = false,
            .toDisplayString = [](const Core::ParameterInfo &, int value) {
                return tr("%Ln cent(s)", nullptr, value);
            }
        });
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
