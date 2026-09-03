// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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

    QVariant ParameterInfoProvider::displayValue(const QVariant &value) const {
        Q_D(const ParameterInfoProvider);
        if (!d->exists || !value.isValid())
            return {};
        const double normalizedValue = value.toDouble();
        if (!std::isfinite(normalizedValue))
            return {};
        return d->info.invokeToDisplayValue(std::clamp(normalizedValue, 0.0, 1.0));
    }

    QString ParameterInfoProvider::displayString(const QVariant &value) const {
        Q_D(const ParameterInfoProvider);
        if (!d->exists || !value.isValid())
            return {};
        const double normalizedValue = value.toDouble();
        if (!std::isfinite(normalizedValue))
            return {};
        return d->info.invokeToDisplayString(std::clamp(normalizedValue, 0.0, 1.0));
    }

    QVariant ParameterInfoProvider::valueFromDisplay(double displayValue) const {
        Q_D(const ParameterInfoProvider);
        if (!d->exists)
            return {};
        const double value = d->info.invokeFromDisplayValue(displayValue);
        if (!std::isfinite(value))
            return {};
        return std::clamp(value, 0.0, 1.0);
    }

    QVariant ParameterInfoProvider::valueFromDspx(const QVariant &value) const {
        if (!value.isValid())
            return {};
        return ParameterInfo::fromDspxModelValue(value.toInt());
    }

    QVariant ParameterInfoProvider::valueToDspx(double value) const {
        Q_D(const ParameterInfoProvider);
        if (!std::isfinite(value))
            value = std::clamp(d->info.defaultValue, 0.0, 1.0);
        return ParameterInfo::toDspxModelValue(value);
    }

}

#include "moc_ParameterInfoProvider.cpp"
