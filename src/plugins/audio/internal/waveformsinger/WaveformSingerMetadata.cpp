// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "WaveformSingerMetadata.h"

#include <cmath>
#include <limits>

#include <QJsonObject>
#include <QLocale>
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
            .defaultValue = 1.0,
            .fillMode = Core::ParameterInfo::FillMode::BottomFill,
            .valueType = Core::ParameterInfo::ValueType::Relative,
            .divisionValue = 0.125,
            .showDefaultValue = false,
            .showDivision = true,
            .toDisplayValue = [](const Core::ParameterInfo &, double value) {
                return value <= 0.0 ? -std::numeric_limits<double>::infinity()
                                    : 20.0 * std::log10(value);
            },
            .fromDisplayValue = [](const Core::ParameterInfo &, double value) {
                return std::pow(10.0, value / 20.0);
            },
            .toDisplayString = [](const Core::ParameterInfo &, double value) {
                if (value <= 0.0)
                    return QStringLiteral("−∞") + tr(" dB");
                return QLocale().toString(20.0 * std::log10(value), 'f', 3) + tr(" dB");
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
        info.setBackgroundUrl(QUrl(QStringLiteral("qrc:/diffscope/audio/singeravatar/waveform_portrait.svg")));
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
