#include "ArchitectureInfo.h"
#include "ArchitectureInfo_p.h"

namespace Core {

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
               (d->name == other.d->name && d->parameters.keys() == other.d->parameters.keys() &&
                d->defaultExtra == other.d->defaultExtra &&
                d->controlPanelComponent == other.d->controlPanelComponent);
    }

    bool ArchitectureInfo::operator!=(const ArchitectureInfo &other) const {
        return !(*this == other);
    }

}
