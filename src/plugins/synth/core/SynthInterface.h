// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHINTERFACE_H
#define DIFFSCOPE_SYNTH_SYNTHINTERFACE_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QStringList>
#include <QUuid>

#include <synth/ServiceTypes.h>
#include <synth/synthglobal.h>

class QQmlComponent;

namespace Synth {

    namespace Internal {
        class ArchitecturePage;
        class SynthService;
    }

    class SynthesisTaskManager;
    class SynthInterfacePrivate;

    class SYNTH_EXPORT SynthInterface : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SynthInterface)

    public:
        ~SynthInterface() override;

        static SynthInterface *instance();

        QList<ServiceInstanceConfiguration> serviceInstances() const;
        ServiceInstanceDetails serviceInstanceDetails(const QUuid &id) const;
        bool containsServiceInstance(const QUuid &id) const;
        SynthesisTaskManager *taskManager() const;

        // Registers built-in control panels and returns whether registration succeeded.
        // Rejects an empty, padded, or duplicate ID and null components. Component ownership remains with the caller.
        bool registerBuiltInArchitecture(const QString &architectureId, QQmlComponent *architectureControlPanelComponent, QQmlComponent *singerControlPanelComponent);
        QStringList builtInArchitectureIds() const;
        bool isBuiltInArchitecture(const QString &architectureId) const;
        QQmlComponent *builtInArchitecturePanel(const QString &architectureId) const;
        QQmlComponent *builtInSingerPanel(const QString &architectureId) const;

    Q_SIGNALS:
        void serviceInstancesChanged();
        void serviceInstanceDetailsChanged(const QUuid &id);
        void controlPanelsChanged();

    private:
        friend class Internal::ArchitecturePage;
        friend class Internal::SynthService;

        explicit SynthInterface(QObject *parent = nullptr);
        void setServiceInstances(const QList<ServiceInstanceConfiguration> &instances);
        void setServiceInstanceDetails(const ServiceInstanceDetails &details);
        void removeServiceInstanceDetails(const QUuid &id);
        void clearParameterRuntime();
        void setTaskManager(SynthesisTaskManager *taskManager);
        void reloadControlPanels();

        QScopedPointer<SynthInterfacePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHINTERFACE_H
