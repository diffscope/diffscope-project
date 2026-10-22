// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageInstallModel.h"

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QLoggingCategory>
#include <QMap>
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

        using Version = std::array<quint64, 4>;

        std::optional<Version> packageVersion(const QString &text) {
            const auto parts = text.split(u'.');
            if (parts.size() != 4)
                return {};
            Version version;
            for (int i = 0; i < 4; ++i) {
                bool ok;
                version[i] = parts.at(i).toULongLong(&ok);
                if (!ok)
                    return {};
            }
            return version;
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
            return packageLabel(index.row());
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

    QString PackageInstallModel::progressText() const {
        return m_progressText;
    }

    QString PackageInstallModel::progressDetail() const {
        return m_progressDetail;
    }

    double PackageInstallModel::progress() const {
        return m_progress;
    }

    bool PackageInstallModel::progressIndeterminate() const {
        return m_progressIndeterminate;
    }

    int PackageInstallModel::activeRow() const {
        return m_activeRow;
    }

    QString PackageInstallModel::packageLabel(int row) const {
        const auto &entry = m_entries.at(row);
        return entry.valid ? tr("%1 (%2)").arg(entry.name.isEmpty() ? entry.id : entry.name, identity(entry.id, entry.version)) : tr("Unknown package");
    }

    bool PackageInstallModel::attemptedInstallation() const {
        return m_attemptedInstallation;
    }

    void PackageInstallModel::addFiles(const QList<QUrl> &files) {
        if (m_busy)
            return;
        bool added = false;
        for (const auto &url : files) {
            if (!url.isLocalFile())
                continue;
            const auto path = normalizedPath(url.toLocalFile());
            bool found = false;
            for (const auto &entry : std::as_const(m_entries)) {
                if (pathKey(entry.path) == pathKey(path)) {
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
            added = true;
        }
        if (added)
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

    DspmResult PackageInstallModel::runInstallation(bool dryRun, bool overwrite) {
        auto arguments = m_arguments;
        arguments.append({QStringLiteral("install"), QStringLiteral("--file-only")});
        if (dryRun)
            arguments.append(QStringLiteral("--dry-run"));
        if (overwrite)
            arguments.append(QStringLiteral("--overwrite-existing"));
        arguments.append(QStringLiteral("--"));
        for (const auto &entry : m_entries)
            arguments.append(entry.path);
        setActivity(dryRun ? tr("Checking packages...") : tr("Installing packages..."));
        QByteArray pendingOutput;
        const auto outputReceived = [this, &pendingOutput](const QByteArray &output) {
            pendingOutput.append(output);
            qsizetype lineEnd;
            while ((lineEnd = pendingOutput.indexOf('\n')) >= 0) {
                const auto line = pendingOutput.first(lineEnd);
                pendingOutput.remove(0, lineEnd + 1);
                const auto document = QJsonDocument::fromJson(line);
                if (document.isObject())
                    updateProgress(document.object());
            }
        };
        const auto command = runCommandLineTool(m_executablePath, arguments, m_timeoutSeconds, {}, outputReceived);
        return parseDspmMutationResult(command, QStringLiteral("install"), dryRun);
    }

    void PackageInstallModel::setActivity(const QString &text, int row) {
        m_progressText = text;
        m_progressDetail.clear();
        m_progress = 0;
        m_progressIndeterminate = true;
        m_activeRow = row;
        Q_EMIT progressChanged();
    }

    void PackageInstallModel::updateProgress(const QJsonObject &record) {
        if (record.value(QStringLiteral("command")).toString() != QStringLiteral("install") || record.value(QStringLiteral("event")).toString() != QStringLiteral("progress"))
            return;
        const auto data = record.value(QStringLiteral("data")).toObject();
        const auto phase = data.value(QStringLiteral("phase")).toString();
        const auto target = data.value(QStringLiteral("target")).toString();
        int row = -1;
        for (int i = 0; i < m_entries.size(); ++i) {
            const auto &entry = m_entries.at(i);
            if ((phase == QStringLiteral("inspect") && pathKey(entry.path) == pathKey(target)) ||
                ((phase == QStringLiteral("extract") || phase == QStringLiteral("verify")) && identity(entry.id, entry.version) == target)) {
                row = i;
                break;
            }
        }
        const auto label = row >= 0 ? packageLabel(row) : target;
        if (phase == QStringLiteral("inspect"))
            m_progressText = tr("Inspecting %1").arg(label);
        else if (phase == QStringLiteral("extract"))
            m_progressText = tr("Extracting %1").arg(label);
        else if (phase == QStringLiteral("verify"))
            m_progressText = tr("Verifying %1").arg(label);
        else if (phase == QStringLiteral("resolve"))
            m_progressText = tr("Checking dependencies...");
        else if (phase == QStringLiteral("commit"))
            m_progressText = tr("Saving installed packages...");
        else if (phase == QStringLiteral("lock-wait"))
            m_progressText = tr("Waiting for another package operation...");
        else
            m_progressText = phase;
        m_activeRow = row;
        const auto current = data.value(QStringLiteral("current")).toInteger();
        const auto total = data.value(QStringLiteral("total")).toInteger();
        m_progressIndeterminate = total <= 0;
        m_progress = total > 0 ? std::clamp(static_cast<double>(current) / static_cast<double>(total), 0.0, 1.0) : 0;
        QStringList details;
        if (row >= 0)
            details.append(tr("Package %L1 of %L2").arg(row + 1).arg(m_entries.size()));
        const auto filePath = data.value(QStringLiteral("filePath")).toString();
        if (!filePath.isEmpty())
            details.append(filePath);
        if (total > 0)
            details.append(tr("%1 of %2 (%L3%)").arg(QLocale().formattedDataSize(current), QLocale().formattedDataSize(total)).arg(static_cast<int>(m_progress * 100)));
        const auto filesTotal = data.value(QStringLiteral("filesTotal")).toInteger();
        if (filesTotal > 0)
            details.append(tr("%L1 of %L2 files").arg(data.value(QStringLiteral("filesDone")).toInteger()).arg(filesTotal));
        m_progressDetail = details.join(u'\n');
        Q_EMIT progressChanged();
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

    void PackageInstallModel::validate() {
        if (m_busy)
            return;
        m_busy = true;
        m_errorMessage.clear();
        Q_EMIT stateChanged();
        const auto guard = qScopeGuard([this] {
            m_busy = false;
            setActivity({});
            notifyEntries();
            Q_EMIT stateChanged();
        });
        validateEntries();
    }

    void PackageInstallModel::validateEntries() {
        m_errorMessage.clear();
        m_planValid = false;
        if (m_entries.isEmpty())
            return;
        QStringList inspectionErrors;
        QHash<QString, QList<int>> identities;
        for (int row = 0; row < m_entries.size(); ++row) {
            auto &entry = m_entries[row];
            if (!entry.inspected) {
                setActivity(tr("Inspecting %1").arg(entry.path), row);
                const auto result = query({QStringLiteral("info"), QStringLiteral("--file"), QStringLiteral("--"), entry.path});
                const auto info = result.data.toObject();
                const auto package = info.value(QStringLiteral("package")).toObject();
                entry.package = package;
                entry.id = package.value(QStringLiteral("id")).toString();
                entry.version = package.value(QStringLiteral("version")).toString();
                entry.name = localizedPackageText(package.value(QStringLiteral("name")));
                entry.hash = info.value(QStringLiteral("hash")).toString();
                entry.valid = result.ok && result.command == QStringLiteral("info") && !entry.id.isEmpty() && !entry.version.isEmpty() && validHash(entry.hash);
                entry.inspected = true;
                entry.inspectionError.clear();
                if (!entry.valid) {
                    entry.inspectionError = result.ok ? tr("The package manager returned invalid package information.") : result.errorMessage;
                    qCWarning(lcPackageInstall) << "Could not inspect package" << entry.path << result.errorMessage;
                }
            }
            entry.status = entry.valid ? Ready : InvalidPackage;
            if (entry.valid)
                identities[identity(entry.id, entry.version)].append(row);
            else
                inspectionErrors.append(tr("Could not inspect %1:\n%2").arg(entry.path, entry.inspectionError));
        }
        for (const auto &rows : std::as_const(identities))
            if (rows.size() > 1) {
                for (const auto row : rows)
                    m_entries[row].status = BatchConflict;
                const auto &entry = m_entries.at(rows.first());
                inspectionErrors.append(tr("Multiple selected files identify %1.").arg(identity(entry.id, entry.version)));
            }
        m_errorMessage = inspectionErrors.join(QStringLiteral("\n\n"));

        setActivity(tr("Reading installed packages..."));
        const auto installed = query({QStringLiteral("list")});
        if (!installed.ok || installed.command != QStringLiteral("list") || !installed.data.isArray()) {
            if (!m_errorMessage.isEmpty())
                m_errorMessage.append(QStringLiteral("\n\n"));
            m_errorMessage.append(tr("Could not read installed packages:\n%1").arg(installed.ok ? tr("The package manager returned invalid package information.") : installed.errorMessage));
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
                if (!m_errorMessage.isEmpty())
                    m_errorMessage.append(QStringLiteral("\n\n"));
                m_errorMessage.append(tr("The package manager returned invalid package information."));
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
            m_errorMessage = tr("Could not prepare package installation:\n\n%1").arg(plan.errorMessage);
            if (plan.errorCode == QStringLiteral("DEPENDENCY_ERROR")) {
                const auto missing = missingDependencies(installed.data.toArray());
                if (!missing.isEmpty())
                    m_errorMessage.prepend(tr("Missing or incompatible dependencies:\n%1\n\n").arg(missing.join(u'\n')));
            }
            qCWarning(lcPackageInstall) << m_errorMessage << plan.errorMessage;
            return;
        }
        QString actionError;
        m_planValid = applyActions(plan.data.toObject().value(QStringLiteral("actions")).toArray(), true, &actionError);
        if (!m_planValid)
            m_errorMessage = tr("The package manager returned an invalid installation plan:\n%1").arg(actionError);
    }

    QStringList PackageInstallModel::missingDependencies(const QJsonArray &installed) const {
        QMap<QString, QJsonObject> packages;
        for (const auto &value : installed) {
            const auto package = value.toObject();
            packages.insert(identity(package.value(QStringLiteral("id")).toString(), package.value(QStringLiteral("version")).toString()), package);
        }
        for (const auto &entry : m_entries)
            if (entry.valid && entry.status != AlreadyInstalled)
                packages.insert(identity(entry.id, entry.version), entry.package);
        QHash<QString, QList<QJsonObject>> providers;
        for (const auto &package : std::as_const(packages))
            providers[package.value(QStringLiteral("id")).toString()].append(package);
        QStringList missing;
        for (auto it = packages.cbegin(); it != packages.cend(); ++it) {
            for (const auto &value : it->value(QStringLiteral("dependencies")).toArray()) {
                const auto dependency = value.toObject();
                const auto id = dependency.value(QStringLiteral("id")).toString();
                const auto versionText = dependency.value(QStringLiteral("version")).toString();
                const auto required = packageVersion(versionText);
                if (!required)
                    continue;
                bool found = false;
                for (const auto &provider : providers.value(id)) {
                    const auto minimum = packageVersion(provider.value(QStringLiteral("compatVersion")).toString());
                    const auto maximum = packageVersion(provider.value(QStringLiteral("version")).toString());
                    if (minimum && maximum && *minimum <= *required && *required <= *maximum) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    missing.append(tr("%1 requires %2.").arg(it.key(), identity(id, versionText)));
            }
        }
        return missing;
    }

    bool PackageInstallModel::applyActions(const QJsonArray &actions, bool overwrite, QString *errorMessage) {
        if (actions.size() != m_entries.size()) {
            *errorMessage = tr("Expected %L1 package actions, but received %L2.").arg(m_entries.size()).arg(actions.size());
            return false;
        }
        QHash<QString, int> rows;
        for (int row = 0; row < m_entries.size(); ++row)
            rows.insert(pathKey(m_entries.at(row).path), row);
        QSet<int> seen;
        for (const auto &value : actions) {
            const auto action = value.toObject();
            const auto invalidAction = [&] {
                *errorMessage = tr("Unexpected installation action:\n%1").arg(QString::fromUtf8(QJsonDocument(action).toJson(QJsonDocument::Indented)));
                return false;
            };
            const auto file = action.value(QStringLiteral("file")).toString();
            const auto it = rows.constFind(pathKey(file));
            if (file.isEmpty() || it == rows.cend() || seen.contains(*it))
                return invalidAction();
            auto &entry = m_entries[*it];
            const auto package = action.value(QStringLiteral("package")).toObject();
            if (package.value(QStringLiteral("id")).toString() != entry.id || package.value(QStringLiteral("version")).toString() != entry.version || action.value(QStringLiteral("hash")).toString() != entry.hash) {
                *errorMessage = tr("The installation action does not match %1:\n%2").arg(packageLabel(*it), QString::fromUtf8(QJsonDocument(action).toJson(QJsonDocument::Indented)));
                return false;
            }
            const auto operation = action.value(QStringLiteral("action")).toString();
            if (operation == QStringLiteral("skip"))
                entry.status = AlreadyInstalled;
            else if (operation == QStringLiteral("overwrite") && overwrite)
                entry.status = OverwriteRequired;
            else if (operation == QStringLiteral("install"))
                entry.status = Ready;
            else
                return invalidAction();
            seen.insert(*it);
        }
        return true;
    }

    bool PackageInstallModel::install(QWindow *parent) {
        if (!canInstall())
            return false;
        m_busy = true;
        m_errorMessage.clear();
        Q_EMIT stateChanged();
        const auto guard = qScopeGuard([this] {
            m_busy = false;
            setActivity({});
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
        QString actionError;
        if (result.ok && applyActions(result.data.toObject().value(QStringLiteral("actions")).toArray(), overwrite, &actionError))
            return true;
        QStringList details{m_progressText};
        if (!m_progressDetail.isEmpty())
            details.append(m_progressDetail);
        details.append(result.ok ? actionError : result.errorMessage);
        m_errorMessage = tr("Package installation failed:\n\n%1").arg(details.join(QStringLiteral("\n\n")));
        qCWarning(lcPackageInstall) << m_errorMessage;
        return false;
    }

}
