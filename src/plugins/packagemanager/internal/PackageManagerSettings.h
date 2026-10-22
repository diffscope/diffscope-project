// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERSETTINGS_H
#define DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERSETTINGS_H

#include <qqmlintegration.h>

#include <QObject>

namespace AppInstall {
    class InstallationManager;
}

class QQmlEngine;
class QJSEngine;

namespace PackageManager {

    class PackageManagerPlugin;

    class PackageManagerSettings : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        Q_PROPERTY(QString dspmPath READ dspmPath NOTIFY dspmPathChanged)
        Q_PROPERTY(QString packageDir READ packageDir WRITE setPackageDir NOTIFY packageDirChanged)
        Q_PROPERTY(int timeoutSeconds READ timeoutSeconds WRITE setTimeoutSeconds NOTIFY timeoutSecondsChanged)

    public:
        ~PackageManagerSettings() override;

        static PackageManagerSettings *instance();

        static inline PackageManagerSettings *create(QQmlEngine *, QJSEngine *) {
            return instance();
        }

        static QString dspmPath();

        AppInstall::InstallationManager *installationManager() const;

        static QString packageDir();
        static void setPackageDir(const QString &packageDir);

        static int timeoutSeconds();
        static void setTimeoutSeconds(int timeoutSeconds);

        static QString defaultPackageDir();

        void load();
        void save() const;

    Q_SIGNALS:
        void dspmPathChanged();
        void packageDirChanged();
        void timeoutSecondsChanged();

    private:
        friend class PackageManagerPlugin;
        explicit PackageManagerSettings(const QString &pluginLocation, QObject *parent = nullptr);

        AppInstall::InstallationManager *m_installations{};
        QString m_packageDir;
        int m_timeoutSeconds{30};
    };

}

#endif // DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERSETTINGS_H
