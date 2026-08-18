// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthInterface.h"

#include <algorithm>
#include <utility>

#include <QSet>

#include <synth/SynthesisTaskManager.h>
#include <synth/internal/BuiltinParameterConfigurations.h>
#include <synth/internal/ParameterRuntimeRegistry.h>
#include <synth/private/SynthInterface_p.h>

namespace Synth {

    namespace {

        SynthInterface *s_instance{};

    }

    SynthInterface::SynthInterface(QObject *parent)
        : QObject(parent), d_ptr(new SynthInterfacePrivate(this)) {
        Q_ASSERT(!s_instance);
        s_instance = this;
        for (const auto &configuration : Internal::BuiltinParameterConfigurations::all())
            registerBuiltinParameterConfiguration(configuration);
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

    SynthInterface::BuiltinParameterRegistrationResult
    SynthInterface::registerBuiltinParameterConfiguration(const ParameterConfiguration &configuration, QString *errorMessage) {
        Q_D(SynthInterface);
        if (configuration.id() == QStringLiteral("pitch")) {
            if (errorMessage)
                *errorMessage = tr("pitch is reserved and cannot be configured");
            return ReservedPitch;
        }
        QStringList errors;
        if (!configuration.validate(&errors)) {
            if (errorMessage)
                *errorMessage = errors.join(u'\n');
            return Invalid;
        }
        if (d->builtinParameters.contains(configuration.id())) {
            if (errorMessage)
                *errorMessage = tr("A built-in parameter with this ID is already registered");
            return AlreadyRegistered;
        }
        d->builtinParameters.insert(configuration.id(), configuration);
        emit builtinParameterConfigurationsChanged();
        return Registered;
    }

    QList<ParameterConfiguration> SynthInterface::builtinParameterConfigurations() const {
        Q_D(const SynthInterface);
        auto result = d->builtinParameters.values();
        std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
            if (left.architectureId() != right.architectureId())
                return left.architectureId() < right.architectureId();
            return left.id() < right.id();
        });
        return result;
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
        emit serviceInstancesChanged();
        for (const auto &id : std::as_const(changedDetailIds))
            emit serviceInstanceDetailsChanged(id);
        for (const auto &id : std::as_const(removedDetailIds))
            emit serviceInstanceDetailsChanged(id);
    }

    void SynthInterface::setServiceInstanceDetails(const ServiceInstanceDetails &details) {
        Q_D(SynthInterface);
        const auto id = details.configuration().id();
        if (id.isNull() || d->serviceDetails.value(id) == details)
            return;
        d->serviceDetails.insert(id, details);
        emit serviceInstanceDetailsChanged(id);
    }

    void SynthInterface::removeServiceInstanceDetails(const QUuid &id) {
        Q_D(SynthInterface);
        if (!d->serviceDetails.remove(id))
            return;
        emit serviceInstanceDetailsChanged(id);
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
