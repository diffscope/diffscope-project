// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_APPINSTALL_INSTALLATIONMANAGER_P_H
#define DIFFSCOPE_APPINSTALL_INSTALLATIONMANAGER_P_H

#include <appinstall/InstallationManager.h>

#include <QHash>
#include <QPointer>
#include <QStringList>

class QTimer;

namespace AppInstall {

    class DownloadCatalog;

    class InstallationManagerPrivate {
        Q_DECLARE_PUBLIC(InstallationManager)
    public:
        InstallationManager *q_ptr{};
        ProductConfiguration product;
        InstallationSelection selection;
        QTimer *versionTimer{};
        QPointer<DownloadCatalog> updateCatalog;
        QStringList pendingVersionPaths;
        QString versionPath;
        QHash<QString, QString> versions;
        VersionProbeCancellation cancelVersionProbe;
        quint64 versionRequestSerial{};
        bool busy{};
        bool shutdown{};

        void startNextVersionQuery();
        void completeVersionQuery(const QString &version);
        void loadSettings();
        void saveSettings() const;
        QString downloadedDirectory(const QString &id) const;
        void cancelVersionProbes(const QString &directory);
    };

}

#endif // DIFFSCOPE_APPINSTALL_INSTALLATIONMANAGER_P_H
