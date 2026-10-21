// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERPAGE_H
#define DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERPAGE_H

#include <QUrl>
#include <QVariant>

#include <CoreApi/isettingpage.h>

#include <synth/internal/DsspServerManager.h>

namespace Synth::Internal {

    class DsspDeviceCatalog;

    class DsspServerPage : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QVariantMap configuration READ configuration NOTIFY configurationChanged)
        Q_PROPERTY(QVariantList installations READ installations NOTIFY installationsChanged)
        Q_PROPERTY(QVariantMap selectedDevice READ selectedDevice NOTIFY configurationChanged)
        Q_PROPERTY(QObject *deviceCatalog READ deviceCatalog CONSTANT)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
        Q_PROPERTY(QString logsLocation READ logsLocation CONSTANT)
    public:
        explicit DsspServerPage(DsspServerManager *manager, QObject *parent = nullptr);
        ~DsspServerPage() override;

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        QVariantMap configuration() const;
        DsspServerConfiguration serverConfiguration() const;
        QVariantList installations() const;
        QVariantMap selectedDevice() const;
        QObject *deviceCatalog() const;
        QString errorMessage() const;
        QString logsLocation() const;

        Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
        Q_INVOKABLE void addLocalInstallation(const QUrl &url);
        Q_INVOKABLE void removeLocalInstallation();
        Q_INVOKABLE void downloadInstallation();
        Q_INVOKABLE void uninstallInstallation();
        Q_INVOKABLE void refreshDevices();
        Q_INVOKABLE void normalizePackageDirectory();
        Q_INVOKABLE QString localFilePath(const QUrl &url) const;

    Q_SIGNALS:
        void configurationChanged();
        void installationsChanged();
        void errorMessageChanged();

    private:
        void setErrorMessage(const QString &message);

        DsspServerManager *m_manager{};
        DsspDeviceCatalog *m_devices{};
        DsspServerConfiguration m_configuration;
        QObject *m_widget{};
        QString m_errorMessage;
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERPAGE_H
