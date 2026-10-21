// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageInstallModel.h"

#include <utility>

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSet>
#include <QWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <packagemanager/internal/PackageManagerSettings.h>

namespace PackageManager {

    Q_STATIC_LOGGING_CATEGORY(lcPackageInstall, "diffscope.packagemanager.install")

    namespace {

        QString normalizedPath(const QString &path) {
            return QDir::toNativeSeparators(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
        }

        QString pathKey(const QString &path) {
            const auto normalized = normalizedPath(path);
#ifdef Q_OS_WIN
            return normalized.toCaseFolded();
#else
            return normalized;
#endif
        }

        QString identity(const QString &id, const QString &version) {
            return id + u'=' + version;
        }

        bool validHash(const QString &hash) {
            static const QRegularExpression pattern(QStringLiteral("\\A[0-9a-f]{128}\\z"));
            return pattern.match(hash).hasMatch();
        }

    }

    PackageInstallModel::PackageInstallModel(QObject *parent)
        : QAbstractListModel(parent), m_executablePath(PackageManagerSettings::dspmPath()), m_timeoutSeconds(PackageManagerSettings::timeoutSeconds()) {
        const auto directory = PackageManagerSettings::packageDir();
        m_arguments = {QStringLiteral("--json"), QStringLiteral("--packages-dir"), directory.isEmpty() ? PackageManagerSettings::defaultPackageDir() : directory};
    }

    PackageInstallModel::~PackageInstallModel() = default;

    int PackageInstallModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_entries.size();
    }

    QVariant PackageInstallModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
            return {};
        const auto &entry = m_entries.at(index.row());
        switch (role) {
        case FilePathRole:
            return entry.path;
        case PackageLabelRole:
            return entry.valid ? tr("%1 (%2)").arg(entry.name.isEmpty() ? entry.id : entry.name, identity(entry.id, entry.version)) : tr("Unknown package");
        case StatusRole:
            return entry.status;
        default:
            return {};
        }
    }

    QHash<int, QByteArray> PackageInstallModel::roleNames() const {
        return {{FilePathRole, "filePath"}, {PackageLabelRole, "packageLabel"}, {StatusRole, "status"}};
    }

    bool PackageInstallModel::busy() const {
        return m_busy;
    }

    bool PackageInstallModel::canInstall() const {
        return !m_busy && m_planValid && !m_entries.isEmpty() && !hasErrors();
    }

    QString PackageInstallModel::errorMessage() const {
        return m_errorMessage;
    }

    bool PackageInstallModel::attemptedInstallation() const {
        return m_attemptedInstallation;
    }

    void PackageInstallModel::addFiles(const QList<QUrl> &files) {
        if (m_busy)
            return;
        for (const auto &url : files) {
            if (!url.isLocalFile())
                continue;
            const auto path = normalizedPath(url.toLocalFile());
            bool found = false;
            for (auto &entry : m_entries) {
                if (pathKey(entry.path) == pathKey(path)) {
                    entry.inspected = false;
                    found = true;
                    break;
                }
            }
            if (found)
                continue;
            const auto row = m_entries.size();
            beginInsertRows({}, row, row);
            Entry entry;
            entry.path = path;
            m_entries.append(entry);
            endInsertRows();
        }
        validate();
    }

    void PackageInstallModel::removeFile(int row) {
        if (m_busy || row < 0 || row >= m_entries.size())
            return;
        beginRemoveRows({}, row, row);
        m_entries.removeAt(row);
        endRemoveRows();
        validate();
    }

    DspmResult PackageInstallModel::query(const QStringList &arguments) const {
        auto commandArguments = m_arguments;
        commandArguments.append(arguments);
        return parseDspmQueryResult(runCommandLineTool(m_executablePath, commandArguments, m_timeoutSeconds));
    }

    DspmResult PackageInstallModel::runInstallation(bool dryRun, bool overwrite) const {
        auto arguments = m_arguments;
        arguments.append({QStringLiteral("install"), QStringLiteral("--file-only")});
        if (dryRun)
            arguments.append(QStringLiteral("--dry-run"));
        if (overwrite)
            arguments.append(QStringLiteral("--overwrite-existing"));
        arguments.append(QStringLiteral("--"));
        for (const auto &entry : m_entries)
            arguments.append(entry.path);
        return parseDspmMutationResult(runCommandLineTool(m_executablePath, arguments, m_timeoutSeconds), QStringLiteral("install"), dryRun);
    }

    bool PackageInstallModel::hasErrors() const {
        for (const auto &entry : m_entries)
            if (!entry.valid || entry.status == BatchConflict)
                return true;
        return false;
    }

    void PackageInstallModel::notifyEntries() {
        if (!m_entries.isEmpty())
            Q_EMIT dataChanged(index(0, 0), index(m_entries.size() - 1, 0));
    }

    void PackageInstallModel::validate(bool reinspect) {
        if (m_busy)
            return;
        m_busy = true;
        Q_EMIT stateChanged();
        const auto guard = qScopeGuard([this] {
            m_busy = false;
            notifyEntries();
            Q_EMIT stateChanged();
        });
        validateEntries(reinspect);
    }

    void PackageInstallModel::validateEntries(bool reinspect) {
        m_errorMessage.clear();
        m_planValid = false;
        if (m_entries.isEmpty())
            return;
        QHash<QString, QList<int>> identities;
        for (int row = 0; row < m_entries.size(); ++row) {
            auto &entry = m_entries[row];
            if (reinspect || !entry.inspected) {
                const auto result = query({QStringLiteral("info"), QStringLiteral("--file"), QStringLiteral("--"), entry.path});
                const auto info = result.data.toObject();
                const auto package = info.value(QStringLiteral("package")).toObject();
                entry.id = package.value(QStringLiteral("id")).toString();
                entry.version = package.value(QStringLiteral("version")).toString();
                entry.name = localizedPackageText(package.value(QStringLiteral("name")));
                entry.hash = info.value(QStringLiteral("hash")).toString();
                entry.valid = result.ok && result.command == QStringLiteral("info") && !entry.id.isEmpty() && !entry.version.isEmpty() && validHash(entry.hash);
                entry.inspected = true;
                if (!entry.valid)
                    qCWarning(lcPackageInstall) << "Could not inspect package" << entry.path << result.errorMessage;
            }
            entry.status = entry.valid ? Ready : InvalidPackage;
            if (entry.valid)
                identities[identity(entry.id, entry.version)].append(row);
        }
        for (const auto &rows : std::as_const(identities))
            if (rows.size() > 1)
                for (const auto row : rows)
                    m_entries[row].status = BatchConflict;

        const auto installed = query({QStringLiteral("list")});
        if (!installed.ok || installed.command != QStringLiteral("list") || !installed.data.isArray()) {
            m_errorMessage = tr("Could not read installed packages.");
            qCWarning(lcPackageInstall) << m_errorMessage << installed.errorMessage;
            return;
        }
        QHash<QString, QString> installedHashes;
        for (const auto &value : installed.data.toArray()) {
            const auto package = value.toObject();
            const auto id = package.value(QStringLiteral("id")).toString();
            const auto version = package.value(QStringLiteral("version")).toString();
            const auto hash = package.value(QStringLiteral("hash")).toString();
            if (id.isEmpty() || version.isEmpty() || !validHash(hash)) {
                m_errorMessage = tr("Could not read installed packages.");
                return;
            }
            installedHashes.insert(identity(id, version), hash);
        }
        for (auto &entry : m_entries) {
            if (!entry.valid || entry.status == BatchConflict)
                continue;
            const auto it = installedHashes.constFind(identity(entry.id, entry.version));
            if (it != installedHashes.cend())
                entry.status = *it == entry.hash ? AlreadyInstalled : OverwriteRequired;
        }
        if (hasErrors())
            return;
        const auto plan = runInstallation(true, true);
        if (!plan.ok) {
            m_errorMessage = plan.errorCode == QStringLiteral("DEPENDENCY_ERROR") ? tr("Required dependencies are missing or incompatible.") : tr("The selected packages cannot be installed together.");
            qCWarning(lcPackageInstall) << m_errorMessage << plan.errorMessage;
            return;
        }
        m_planValid = applyActions(plan.data.toObject().value(QStringLiteral("actions")).toArray(), true);
        if (!m_planValid)
            m_errorMessage = tr("The package manager returned an invalid installation plan.");
    }

    bool PackageInstallModel::applyActions(const QJsonArray &actions, bool overwrite) {
        if (actions.size() != m_entries.size())
            return false;
        QHash<QString, int> rows;
        for (int row = 0; row < m_entries.size(); ++row)
            rows.insert(pathKey(m_entries.at(row).path), row);
        QSet<int> seen;
        for (const auto &value : actions) {
            const auto action = value.toObject();
            const auto file = action.value(QStringLiteral("file")).toString();
            const auto it = rows.constFind(pathKey(file));
            if (file.isEmpty() || it == rows.cend() || seen.contains(*it))
                return false;
            auto &entry = m_entries[*it];
            const auto package = action.value(QStringLiteral("package")).toObject();
            if (package.value(QStringLiteral("id")).toString() != entry.id || package.value(QStringLiteral("version")).toString() != entry.version || action.value(QStringLiteral("hash")).toString() != entry.hash)
                return false;
            const auto operation = action.value(QStringLiteral("action")).toString();
            if (operation == QStringLiteral("skip"))
                entry.status = AlreadyInstalled;
            else if (operation == QStringLiteral("overwrite") && overwrite)
                entry.status = OverwriteRequired;
            else if (operation == QStringLiteral("install"))
                entry.status = Ready;
            else
                return false;
            seen.insert(*it);
        }
        return true;
    }

    bool PackageInstallModel::install(QWindow *parent) {
        if (!canInstall())
            return false;
        validate(true);
        if (!canInstall())
            return false;
        m_busy = true;
        Q_EMIT stateChanged();
        const auto guard = qScopeGuard([this] {
            m_busy = false;
            notifyEntries();
            Q_EMIT stateChanged();
        });
        bool overwrite = false;
        for (const auto &entry : std::as_const(m_entries))
            overwrite |= entry.status == OverwriteRequired;
        if (overwrite && SVS::MessageBox::question(
                Core::RuntimeInterface::qmlEngine(), parent, tr("Overwrite Existing Packages"),
                tr("Some selected packages have different contents from installed packages with the same identifier. Replace the installed packages?"),
                SVS::SVSCraft::Yes | SVS::SVSCraft::No, SVS::SVSCraft::No
            ) != SVS::SVSCraft::Yes)
            return false;
        m_attemptedInstallation = true;
        const auto result = runInstallation(false, overwrite);
        if (result.ok && applyActions(result.data.toObject().value(QStringLiteral("actions")).toArray(), overwrite))
            return true;
        qCWarning(lcPackageInstall) << "Package installation failed" << result.errorMessage;
        validateEntries(true);
        m_errorMessage = tr("Package installation failed.");
        return false;
    }

}
