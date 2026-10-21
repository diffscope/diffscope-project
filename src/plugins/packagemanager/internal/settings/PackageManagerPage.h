// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERPAGE_H
#define DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERPAGE_H

#include <QObject>
#include <QUrl>

#include <CoreApi/isettingpage.h>

#include <appinstall/InstallationTypes.h>

namespace PackageManager {

    class PackageManagerPage : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QVariantList installations READ installations NOTIFY installationsChanged)
        Q_PROPERTY(QString installationId READ installationId WRITE setInstallationId NOTIFY installationChanged)
        Q_PROPERTY(QString executablePath READ executablePath NOTIFY installationChanged)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
        Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    public:
        explicit PackageManagerPage(QObject *parent = nullptr);
        ~PackageManagerPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        QVariantList installations() const;
        QString installationId() const;
        void setInstallationId(const QString &id);
        QString executablePath() const;
        QString errorMessage() const;
        bool busy() const;

        Q_INVOKABLE void addLocalInstallation(const QUrl &url);
        Q_INVOKABLE void removeLocalInstallation();
        Q_INVOKABLE void downloadInstallation();
        Q_INVOKABLE void uninstallInstallation();
        Q_INVOKABLE QString localFilePath(const QUrl &url);

    Q_SIGNALS:
        void installationsChanged();
        void installationChanged();
        void errorMessageChanged();
        void busyChanged();

    private:
        bool widgetMatches(const QString &word);
        void setErrorMessage(const QString &message);

        AppInstall::InstallationSelection m_selection;
        bool m_validating{};
        QString m_errorMessage;
        QObject *m_widget{};
    };

}

#endif // DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERPAGE_H
