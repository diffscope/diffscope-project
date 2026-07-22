#include "WaveformSingerMetadata.h"

#include <QJsonObject>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <audio/internal/WaveformSingerTypeCatalog.h>

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

    Core::SingerInfo WaveformSingerMetadata::singerInfo() {
        Core::SingerInfo info;
        info.setName(tr("Waveform Synthesizer"));
        info.setMixGroup(QStringLiteral("waveform"));
        info.setAvatarUrl(QUrl(QStringLiteral("qrc:/diffscope/audio/singeravatar/waveform.svg")));
        info.setDefaultExtra(QJsonObject{
            {QStringLiteral("type"), WaveformSingerTypeCatalog::fallbackType()},
        });
        return info;
    }

    bool WaveformSingerMetadata::registerAll(Core::SingerRegistry *registry) {
        if (!registry)
            return false;

        auto controlPanelComponent = new QQmlComponent(
            Core::RuntimeInterface::qmlEngine(),
            QStringLiteral("DiffScope.Audio"),
            QStringLiteral("WaveformSingerControlPanel"),
            registry
        );
        if (controlPanelComponent->isError()) {
            qFatal() << controlPanelComponent->errorString();
        }

        const auto id = architectureId();
        auto architecture = architectureInfo();
        architecture.setControlPanelComponent(controlPanelComponent);
        if (!registry->registerArchitecture(id, architecture))
            return false;

        return registry->registerSinger(id, QStringLiteral("waveform"), singerInfo());
    }

}
