// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_SYNTHESISENVIRONMENTWELCOMEWIZARDPAGEHELPER_H
#define DIFFSCOPE_SYNTH_INTERNAL_SYNTHESISENVIRONMENTWELCOMEWIZARDPAGEHELPER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QtQmlIntegration/qqmlintegration.h>

#include <synth/ServiceTypes.h>

class QAbstractItemModel;

namespace Synth::Internal {

    class DsspServerManager;
    class DsspServerPage;
    class ServiceConfigurationModel;
    class SynthService;

    class SynthesisEnvironmentWelcomeWizardPageHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(bool useLocal READ useLocal WRITE setUseLocal NOTIFY useLocalChanged)
        Q_PROPERTY(QObject *localSettings READ localSettings CONSTANT)
        Q_PROPERTY(QAbstractItemModel *serviceModel READ serviceModel CONSTANT)
        Q_PROPERTY(QUuid currentServiceId READ currentServiceId WRITE setCurrentServiceId NOTIFY connectionStatusChanged)
        Q_PROPERTY(int singerCount READ singerCount NOTIFY singerCountChanged)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
        Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
        Q_PROPERTY(bool connectionFailed READ connectionFailed NOTIFY connectionStatusChanged)
    public:
        explicit SynthesisEnvironmentWelcomeWizardPageHelper(QObject *parent = nullptr);

        bool useLocal() const;
        void setUseLocal(bool useLocal);
        QObject *localSettings() const;
        QAbstractItemModel *serviceModel() const;
        QUuid currentServiceId() const;
        void setCurrentServiceId(const QUuid &id);
        int singerCount() const;
        QString errorMessage() const;
        QString connectionStatus() const;
        bool connectionFailed() const;

        Q_INVOKABLE void enter();
        Q_INVOKABLE void leave();
        Q_INVOKABLE void reloadLocalSettings();
        Q_INVOKABLE void connectServices();

    Q_SIGNALS:
        void useLocalChanged();
        void singerCountChanged();
        void errorMessageChanged();
        void connectionStatusChanged();

    private:
        void applyLocalConfiguration();
        void reloadServiceConfigurations();
        bool applyServices(const QList<ServiceInstanceConfiguration> &configurations);
        void setErrorMessage(const QString &message);

        DsspServerManager *m_server{};
        DsspServerPage *m_localSettings{};
        SynthService *m_service{};
        ServiceConfigurationModel *m_serviceModel{};
        QList<ServiceInstanceConfiguration> m_lastValidServices;
        QUuid m_currentServiceId;
        QString m_errorMessage;
        bool m_active{};
        bool m_reloadingLocal{};
        bool m_applyingLocal{};
        bool m_serviceEdited{};
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_SYNTHESISENVIRONMENTWELCOMEWIZARDPAGEHELPER_H
