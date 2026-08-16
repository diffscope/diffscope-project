// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHINTERFACE_H
#define DIFFSCOPE_SYNTH_SYNTHINTERFACE_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QUuid>

#include <synth/ParameterConfiguration.h>
#include <synth/ServiceTypes.h>
#include <synth/synthglobal.h>

namespace Synth {

    namespace Internal {
        class SynthService;
    }

    class SynthesisTaskManager;
    class SynthInterfacePrivate;

    class SYNTH_EXPORT SynthInterface : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SynthInterface)

    public:
        enum BuiltinParameterRegistrationResult {
            Registered,
            AlreadyRegistered,
            Invalid,
            ReservedPitch,
        };
        Q_ENUM(BuiltinParameterRegistrationResult)

        ~SynthInterface() override;

        static SynthInterface *instance();

        QList<ServiceInstanceConfiguration> serviceInstances() const;
        ServiceInstanceDetails serviceInstanceDetails(const QUuid &id) const;
        bool containsServiceInstance(const QUuid &id) const;
        SynthesisTaskManager *taskManager() const;

        BuiltinParameterRegistrationResult
        registerBuiltinParameterConfiguration(const ParameterConfiguration &configuration, QString *errorMessage = nullptr);
        QList<ParameterConfiguration> builtinParameterConfigurations() const;

    signals:
        void serviceInstancesChanged();
        void serviceInstanceDetailsChanged(const QUuid &id);
        void builtinParameterConfigurationsChanged();

    private:
        friend class Internal::SynthService;

        explicit SynthInterface(QObject *parent = nullptr);
        void setServiceInstances(const QList<ServiceInstanceConfiguration> &instances);
        void setServiceInstanceDetails(const ServiceInstanceDetails &details);
        void removeServiceInstanceDetails(const QUuid &id);
        void clearParameterRuntime();
        void setTaskManager(SynthesisTaskManager *taskManager);

        QScopedPointer<SynthInterfacePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHINTERFACE_H
