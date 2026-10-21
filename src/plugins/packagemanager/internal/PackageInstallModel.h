// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_PACKAGEMANAGER_PACKAGEINSTALLMODEL_H
#define DIFFSCOPE_PACKAGEMANAGER_PACKAGEINSTALLMODEL_H

#include <qqmlintegration.h>

#include <QAbstractListModel>
#include <QJsonArray>
#include <QList>
#include <QStringList>
#include <QUrl>
#include <QWindow>

#include <packagemanager/internal/PackageManagerUtils.h>

namespace PackageManager {

    class PackageInstallModel : public QAbstractListModel {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
        Q_PROPERTY(bool canInstall READ canInstall NOTIFY stateChanged)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    public:
        enum Status {
            Ready,
            Checking,
            InvalidPackage,
            BatchConflict,
            OverwriteRequired,
            AlreadyInstalled,
        };
        Q_ENUM(Status)

        explicit PackageInstallModel(QObject *parent = nullptr);
        ~PackageInstallModel() override;

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

        bool busy() const;
        bool canInstall() const;
        QString errorMessage() const;
        bool attemptedInstallation() const;

        Q_INVOKABLE void addFiles(const QList<QUrl> &files);
        Q_INVOKABLE void removeFile(int row);
        Q_INVOKABLE bool install(QWindow *parent);

    Q_SIGNALS:
        void stateChanged();

    private:
        enum Role {
            FilePathRole = Qt::UserRole + 1,
            PackageLabelRole,
            StatusRole,
        };
        struct Entry {
            QString path;
            QString id;
            QString version;
            QString name;
            QString hash;
            Status status{Checking};
            bool inspected{};
            bool valid{};
        };

        DspmResult query(const QStringList &arguments) const;
        DspmResult runInstallation(bool dryRun, bool overwrite) const;
        void validate(bool reinspect = false);
        void validateEntries(bool reinspect);
        bool applyActions(const QJsonArray &actions, bool overwrite);
        bool hasErrors() const;
        void notifyEntries();

        QList<Entry> m_entries;
        QString m_executablePath;
        QStringList m_arguments;
        int m_timeoutSeconds{};
        bool m_busy{};
        bool m_planValid{};
        bool m_attemptedInstallation{};
        QString m_errorMessage;
    };

}

#endif // DIFFSCOPE_PACKAGEMANAGER_PACKAGEINSTALLMODEL_H
