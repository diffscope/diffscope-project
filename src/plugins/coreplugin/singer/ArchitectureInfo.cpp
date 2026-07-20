#include "ArchitectureInfo.h"
#include "ArchitectureInfo_p.h"

namespace Core {

    ParameterInfo pitchParameterInfo() {
        ParameterInfo info;
        info.bottomValue = 0;
        info.topValue = 12800;
        info.toDisplayValue = [](const ParameterInfo &, int value) {
            return static_cast<double>(value) / 100.0;
        };
        info.fromDisplayValue = [](const ParameterInfo &, double value) {
            return static_cast<int>(value * 100.0);
        };
        info.toDisplayString = [](const ParameterInfo &, int value) {
            return QString::number(static_cast<double>(value) / 100.0);
        };
        return info;
    }

    ParameterInfo transformParameterInfo() {
        ParameterInfo info;
        info.bottomValue = 0;
        info.topValue = 2000;
        info.defaultValue = 1000;
        info.fillMode = ParameterInfo::NoFill;
        info.valueType = ParameterInfo::Relative;
        info.normalize = [](const ParameterInfo &, int value) {
            return static_cast<double>(value) / 2000.0;
        };
        info.denormalize = [](const ParameterInfo &, double value) {
            return static_cast<int>(value * 2000.0);
        };
        info.toDisplayValue = [](const ParameterInfo &, int value) {
            return static_cast<double>(value) / 1000.0;
        };
        info.fromDisplayValue = [](const ParameterInfo &, double value) {
            return static_cast<int>(value * 1000.0);
        };
        info.toDisplayString = [](const ParameterInfo &, int value) {
            return QString::number(static_cast<double>(value) / 1000.0);
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
