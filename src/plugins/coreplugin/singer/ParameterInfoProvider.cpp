#include "ParameterInfoProvider.h"
#include "ParameterInfoProvider_p.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <coreplugin/SingerRegistry.h>

namespace Core {

    void ParameterInfoProviderPrivate::update() {
        Q_Q(ParameterInfoProvider);
        bool newExists = false;
        ParameterInfo newInfo;
        if (parameterId == QStringLiteral("pitch")) {
            newExists = true;
            newInfo = transform ? transformParameterInfo() : pitchParameterInfo();
        } else if (registry && registry->containsArchitecture(architectureId)) {
            const auto parameters = registry->architectureInfo(architectureId).parameters();
            const auto it = parameters.constFind(parameterId);
            if (it != parameters.cend()) {
                newExists = true;
                newInfo = transform ? transformParameterInfo() : it.value();
            }
        }
        if (info != newInfo) {
            info = newInfo;
            Q_EMIT q->infoChanged(info);
        }
        if (exists != newExists) {
            exists = newExists;
            Q_EMIT q->existsChanged(exists);
        }
    }

    void ParameterInfoProviderPrivate::disconnectRegistry() {
        for (const auto &connection : std::as_const(registryConnections))
            QObject::disconnect(connection);
        registryConnections.clear();
    }

    ParameterInfoProvider::ParameterInfoProvider(QObject *parent)
        : QObject(parent), d_ptr(new ParameterInfoProviderPrivate) {
        Q_D(ParameterInfoProvider);
        d->q_ptr = this;
    }

    ParameterInfoProvider::~ParameterInfoProvider() = default;

    SingerRegistry *ParameterInfoProvider::registry() const {
        Q_D(const ParameterInfoProvider);
        return d->registry;
    }

    void ParameterInfoProvider::setRegistry(SingerRegistry *registry) {
        Q_D(ParameterInfoProvider);
        if (d->registry == registry)
            return;
        d->disconnectRegistry();
        d->registry = registry;
        if (registry) {
            const auto updateIfMatching = [this](const QString &architectureId) {
                Q_D(ParameterInfoProvider);
                if (d->architectureId == architectureId)
                    d->update();
            };
            d->registryConnections.append(connect(registry, &SingerRegistry::architectureRegistered,
                                                  this, updateIfMatching));
            d->registryConnections.append(connect(registry, &SingerRegistry::architectureUpdated,
                                                  this, updateIfMatching));
            d->registryConnections.append(connect(registry, &SingerRegistry::architectureRemoved,
                                                  this, updateIfMatching));
            d->registryConnections.append(connect(registry, &QObject::destroyed, this, [this] {
                Q_D(ParameterInfoProvider);
                d->disconnectRegistry();
                d->registry = nullptr;
                Q_EMIT registryChanged(nullptr);
                d->update();
            }));
        }
        Q_EMIT registryChanged(registry);
        d->update();
    }

    QString ParameterInfoProvider::architectureId() const {
        Q_D(const ParameterInfoProvider);
        return d->architectureId;
    }

    void ParameterInfoProvider::setArchitectureId(const QString &architectureId) {
        Q_D(ParameterInfoProvider);
        if (d->architectureId == architectureId)
            return;
        d->architectureId = architectureId;
        Q_EMIT architectureIdChanged(d->architectureId);
        d->update();
    }

    QString ParameterInfoProvider::parameterId() const {
        Q_D(const ParameterInfoProvider);
        return d->parameterId;
    }

    void ParameterInfoProvider::setParameterId(const QString &parameterId) {
        Q_D(ParameterInfoProvider);
        if (d->parameterId == parameterId)
            return;
        d->parameterId = parameterId;
        Q_EMIT parameterIdChanged(d->parameterId);
        d->update();
    }

    bool ParameterInfoProvider::isTransform() const {
        Q_D(const ParameterInfoProvider);
        return d->transform;
    }

    void ParameterInfoProvider::setTransform(bool transform) {
        Q_D(ParameterInfoProvider);
        if (d->transform == transform)
            return;
        d->transform = transform;
        Q_EMIT transformChanged(d->transform);
        d->update();
    }

    ParameterInfo ParameterInfoProvider::info() const {
        Q_D(const ParameterInfoProvider);
        return d->info;
    }

    bool ParameterInfoProvider::exists() const {
        Q_D(const ParameterInfoProvider);
        return d->exists;
    }

    QVariant ParameterInfoProvider::displayValue(const QVariant &rawValue) const {
        Q_D(const ParameterInfoProvider);
        if (!d->exists || !rawValue.isValid())
            return {};
        return d->info.invokeToDisplayValue(rawValue.toInt());
    }

    QString ParameterInfoProvider::displayString(const QVariant &rawValue) const {
        Q_D(const ParameterInfoProvider);
        if (!d->exists || !rawValue.isValid())
            return {};
        return d->info.invokeToDisplayString(rawValue.toInt());
    }

    QVariant ParameterInfoProvider::rawValue(double displayValue) const {
        Q_D(const ParameterInfoProvider);
        if (!d->exists || !std::isfinite(displayValue))
            return {};
        const double bottom = d->info.invokeToDisplayValue(d->info.bottomValue);
        const double top = d->info.invokeToDisplayValue(d->info.topValue);
        if (std::isfinite(bottom) && std::isfinite(top))
            displayValue = std::clamp(displayValue, std::min(bottom, top), std::max(bottom, top));
        const int rawValue = d->info.invokeFromDisplayValue(displayValue);
        return std::clamp(rawValue, std::min(d->info.bottomValue, d->info.topValue),
                          std::max(d->info.bottomValue, d->info.topValue));
    }

}

#include "moc_ParameterInfoProvider.cpp"
