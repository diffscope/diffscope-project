// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthInterface.h"
#include "SynthInterface_p.h"

#include <algorithm>

#include <QSet>

#include <synth/SynthesisTaskManager.h>
#include <synth/internal/ParameterRuntimeRegistry.h>

namespace Synth {

    namespace {

        SynthInterface *s_instance{};

    }

    SynthInterface::SynthInterface(QObject *parent)
        : QObject(parent), d_ptr(new SynthInterfacePrivate(this)) {
        Q_ASSERT(!s_instance);
        s_instance = this;
    }

    SynthInterface::~SynthInterface() {
        if (s_instance == this)
            s_instance = nullptr;
    }

    SynthInterface *SynthInterface::instance() {
        return s_instance;
    }

    QList<ServiceInstanceConfiguration> SynthInterface::serviceInstances() const {
        Q_D(const SynthInterface);
        return d->serviceInstances;
    }

    ServiceInstanceDetails SynthInterface::serviceInstanceDetails(const QUuid &id) const {
        Q_D(const SynthInterface);
        const auto details = d->serviceDetails.constFind(id);
        if (details != d->serviceDetails.cend())
            return *details;
        const auto configuration = std::find_if(
            d->serviceInstances.cbegin(), d->serviceInstances.cend(),
            [&id](const auto &candidate) { return candidate.id() == id; }
        );
        ServiceInstanceDetails result;
        if (configuration != d->serviceInstances.cend()) {
            result.setConfiguration(*configuration);
            result.setHealthStatus(configuration->isEnabled() ? ServiceInstanceDetails::Unknown : ServiceInstanceDetails::Disabled);
        }
        return result;
    }

    bool SynthInterface::containsServiceInstance(const QUuid &id) const {
        Q_D(const SynthInterface);
        return std::any_of(d->serviceInstances.cbegin(), d->serviceInstances.cend(), [&id](const auto &configuration) { return configuration.id() == id; });
    }

    SynthesisTaskManager *SynthInterface::taskManager() const {
        Q_D(const SynthInterface);
        return d->taskManager;
    }

    bool SynthInterface::registerBuiltInArchitecture(const QString &architectureId, QQmlComponent *architectureControlPanelComponent, QQmlComponent *singerControlPanelComponent) {
        Q_D(SynthInterface);
        if (architectureId.isEmpty() || architectureId != architectureId.trimmed() ||
            d->builtInArchitectures.contains(architectureId) || !architectureControlPanelComponent || !singerControlPanelComponent)
            return false;
        d->builtInArchitectures.insert(architectureId, {architectureControlPanelComponent, singerControlPanelComponent});
        connect(architectureControlPanelComponent, &QObject::destroyed, this, &SynthInterface::reloadControlPanels);
        if (singerControlPanelComponent != architectureControlPanelComponent)
            connect(singerControlPanelComponent, &QObject::destroyed, this, &SynthInterface::reloadControlPanels);
        Q_EMIT controlPanelsChanged();
        return true;
    }

    QStringList SynthInterface::builtInArchitectureIds() const {
        Q_D(const SynthInterface);
        auto ids = d->builtInArchitectures.keys();
        ids.sort();
        return ids;
    }

    bool SynthInterface::isBuiltInArchitecture(const QString &architectureId) const {
        Q_D(const SynthInterface);
        return d->builtInArchitectures.contains(architectureId);
    }

    QQmlComponent *SynthInterface::builtInArchitecturePanel(const QString &architectureId) const {
        Q_D(const SynthInterface);
        return d->builtInArchitectures.value(architectureId).architecture;
    }

    QQmlComponent *SynthInterface::builtInSingerPanel(const QString &architectureId) const {
        Q_D(const SynthInterface);
        return d->builtInArchitectures.value(architectureId).singer;
    }

    void SynthInterface::reloadControlPanels() {
        Q_EMIT controlPanelsChanged();
    }

    void SynthInterface::setServiceInstances(const QList<ServiceInstanceConfiguration> &instances) {
        Q_D(SynthInterface);
        if (d->serviceInstances == instances)
            return;
        d->serviceInstances = instances;
        QSet<QUuid> liveIds;
        QList<QUuid> changedDetailIds;
        for (const auto &instance : instances) {
            liveIds.insert(instance.id());
            auto details = d->serviceDetails.find(instance.id());
            if (details != d->serviceDetails.end() && details->configuration() != instance) {
                details->setConfiguration(instance);
                changedDetailIds.append(instance.id());
            }
        }
        QList<QUuid> removedDetailIds;
        for (auto iterator = d->serviceDetails.begin(); iterator != d->serviceDetails.end();) {
            if (!liveIds.contains(iterator.key())) {
                removedDetailIds.append(iterator.key());
                iterator = d->serviceDetails.erase(iterator);
            } else {
                ++iterator;
            }
        }
        Q_EMIT serviceInstancesChanged();
        for (const auto &id : std::as_const(changedDetailIds))
            Q_EMIT serviceInstanceDetailsChanged(id);
        for (const auto &id : std::as_const(removedDetailIds))
            Q_EMIT serviceInstanceDetailsChanged(id);
    }

    void SynthInterface::setServiceInstanceDetails(const ServiceInstanceDetails &details) {
        Q_D(SynthInterface);
        const auto id = details.configuration().id();
        if (id.isNull() || d->serviceDetails.value(id) == details)
            return;
        d->serviceDetails.insert(id, details);
        Q_EMIT serviceInstanceDetailsChanged(id);
    }

    void SynthInterface::removeServiceInstanceDetails(const QUuid &id) {
        Q_D(SynthInterface);
        if (!d->serviceDetails.remove(id))
            return;
        Q_EMIT serviceInstanceDetailsChanged(id);
    }

    void SynthInterface::clearParameterRuntime() {
        Internal::ParameterRuntimeRegistry::instance().clear();
    }

    void SynthInterface::setTaskManager(SynthesisTaskManager *taskManager) {
        Q_D(SynthInterface);
        d->taskManager = taskManager;
    }

}

#include "moc_SynthInterface.cpp"
