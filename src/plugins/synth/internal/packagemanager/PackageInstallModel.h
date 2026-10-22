// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_PACKAGEINSTALLMODEL_H
#define DIFFSCOPE_SYNTH_INTERNAL_PACKAGEINSTALLMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QStringList>
#include <QUrl>
#include <QWindow>
#include <qqmlintegration.h>

#include <synth/internal/PackageManagerUtils.h>

namespace Synth::Internal {

    class PackageInstallModel : public QAbstractListModel {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
        Q_PROPERTY(bool canInstall READ canInstall NOTIFY stateChanged)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
        Q_PROPERTY(QString progressText READ progressText NOTIFY progressChanged)
        Q_PROPERTY(QString progressDetail READ progressDetail NOTIFY progressChanged)
        Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
        Q_PROPERTY(bool progressIndeterminate READ progressIndeterminate NOTIFY progressChanged)
        Q_PROPERTY(int activeRow READ activeRow NOTIFY progressChanged)
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
        QString progressText() const;
        QString progressDetail() const;
        double progress() const;
        bool progressIndeterminate() const;
        int activeRow() const;
        bool attemptedInstallation() const;

        Q_INVOKABLE void addFiles(const QList<QUrl> &files);
        Q_INVOKABLE void removeFile(int row);
        Q_INVOKABLE bool install(QWindow *parent);

    Q_SIGNALS:
        void stateChanged();
        void progressChanged();

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
            QJsonObject package;
            QString inspectionError;
            Status status{Checking};
            bool inspected{};
            bool valid{};
        };

        DspmResult query(const QStringList &arguments) const;
        DspmResult runInstallation(bool dryRun, bool overwrite);
        void validate();
        void validateEntries();
        bool applyActions(const QJsonArray &actions, bool overwrite, QString *errorMessage);
        QStringList missingDependencies(const QJsonArray &installed) const;
        QString packageLabel(int row) const;
        void setActivity(const QString &text, int row = -1);
        void updateProgress(const QJsonObject &record);
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
        QString m_progressText;
        QString m_progressDetail;
        double m_progress{};
        bool m_progressIndeterminate{true};
        int m_activeRow{-1};
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_PACKAGEINSTALLMODEL_H
