// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ArchitectureInfo.h"
#include "ArchitectureInfo_p.h"

#include <algorithm>
#include <cmath>

#include <QLocale>

namespace Core {

    double ParameterInfo::fromDspxModelValue(int value) {
        return std::clamp(static_cast<double>(value) / 8064000.0, 0.0, 1.0);
    }

    int ParameterInfo::toDspxModelValue(double value) {
        if (std::isnan(value))
            value = 0.0;
        return static_cast<int>(std::llround(std::clamp(value, 0.0, 1.0) * 8064000.0));
    }

    ParameterInfo pitchParameterInfo() {
        ParameterInfo info;
        info.defaultValue = 0.0;
        info.toDisplayValue = [](const ParameterInfo &, double value) {
            return value * 128.0;
        };
        info.fromDisplayValue = [](const ParameterInfo &, double value) {
            return value / 128.0;
        };
        info.toDisplayString = [](const ParameterInfo &, double value) {
            return QLocale().toString(value * 128.0);
        };
        return info;
    }

    ParameterInfo transformParameterInfo() {
        ParameterInfo info;
        info.defaultValue = 0.5;
        info.divisionValue = 0.1;
        info.fillMode = ParameterInfo::NoFill;
        info.valueType = ParameterInfo::Relative;
        info.toDisplayValue = [](const ParameterInfo &, double value) {
            return value * 2.0;
        };
        info.fromDisplayValue = [](const ParameterInfo &, double value) {
            return value / 2.0;
        };
        info.toDisplayString = [](const ParameterInfo &, double value) {
            return QLocale().toString(value * 2.0);
        };
        return info;
    }

    ArchitectureInfo::ArchitectureInfo() : d(new ArchitectureInfoData) {
    }

    ArchitectureInfo::ArchitectureInfo(const ArchitectureInfo &other) = default;

    ArchitectureInfo::ArchitectureInfo(ArchitectureInfo &&other) noexcept = default;

    ArchitectureInfo &ArchitectureInfo::operator=(const ArchitectureInfo &other) = default;

    ArchitectureInfo &ArchitectureInfo::operator=(ArchitectureInfo &&other) noexcept = default;

    ArchitectureInfo::~ArchitectureInfo() = default;

    QString ArchitectureInfo::name() const {
        return d->name;
    }

    void ArchitectureInfo::setName(const QString &name) {
        d->name = name;
    }

    ArchitectureInfo::ParameterMap ArchitectureInfo::parameters() const {
        return d->parameters;
    }

    void ArchitectureInfo::setParameters(const ParameterMap &parameters) {
        d->parameters = parameters;
    }

    QJsonValue ArchitectureInfo::defaultExtra() const {
        return d->defaultExtra;
    }

    void ArchitectureInfo::setDefaultExtra(const QJsonValue &defaultExtra) {
        d->defaultExtra = defaultExtra;
    }

    QQmlComponent *ArchitectureInfo::controlPanelComponent() const {
        return d->controlPanelComponent;
    }

    void ArchitectureInfo::setControlPanelComponent(QQmlComponent *component) {
        d->controlPanelComponent = component;
    }

    bool ArchitectureInfo::operator==(const ArchitectureInfo &other) const {
        return d.constData() == other.d.constData() ||
               (d->name == other.d->name && d->parameters == other.d->parameters &&
                d->defaultExtra == other.d->defaultExtra &&
                d->controlPanelComponent == other.d->controlPanelComponent);
    }

    bool ArchitectureInfo::operator!=(const ArchitectureInfo &other) const {
        return !(*this == other);
    }

}
