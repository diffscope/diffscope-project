// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_APPINSTALL_INSTALLATIONMANAGER_H
#define DIFFSCOPE_APPINSTALL_INSTALLATIONMANAGER_H

#include <QObject>
#include <QScopedPointer>

#include <appinstall/InstallationTypes.h>

class QWindow;

namespace AppInstall {

    class InstallationManagerPrivate;

    class APPINSTALL_EXPORT InstallationManager : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(InstallationManager)
        Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
        Q_PROPERTY(QString executablePath READ executablePath NOTIFY selectionChanged)
    public:
        using PrepareRemoval = std::function<bool(const QString &directory, QString *errorMessage)>;

        explicit InstallationManager(const ProductConfiguration &product, QObject *parent = nullptr);
        ~InstallationManager() override;

        ProductConfiguration product() const;
        InstallationSelection selection() const;
        void setSelection(const InstallationSelection &selection);
        QString defaultInstallationId() const;
        QVariantList installations(const InstallationSelection &selection) const;
        QString executablePath() const;
        QString executablePath(const InstallationSelection &selection) const;
        QString channel(const InstallationSelection &selection) const;
        QString version(const InstallationSelection &selection) const;
        bool addLocalInstallation(InstallationSelection &selection, const QString &path, QString *errorMessage) const;
        bool removeLocalInstallation(InstallationSelection &selection) const;
        void refreshVersions(const InstallationSelection &selection);
        DownloadResult download(QWindow *parent);
        void fetchLatestVersions();
        void cancelLatestVersionRequest();
        bool uninstall(const QString &id, QWindow *parent, const PrepareRemoval &prepare = {}, const QString &preparationText = {});
        bool busy() const;
        void shutdown();

        static QString normalizedPath(const QString &path);
        static bool pathIsWithin(const QString &directory, const QString &path);
        static InstallationSelection normalizedSelection(const InstallationSelection &selection);

    Q_SIGNALS:
        void selectionChanged();
        void installationsChanged();
        void busyChanged();
        void installationRemoved(const QString &id);
        void latestVersionsReady(const QVariantMap &versions, const QString &errorMessage);

    private:
        QScopedPointer<InstallationManagerPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_APPINSTALL_INSTALLATIONMANAGER_H
