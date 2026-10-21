// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "InstallationManager.h"
#include "InstallationManager_p.h"

#include <utility>

#include <QDir>
#include <QFileInfo>
#include <QPair>
#include <QPointer>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QUuid>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <appinstall/private/DownloadCatalog_p.h>
#include <appinstall/private/Installer_p.h>

namespace AppInstall {

    InstallationManager::InstallationManager(const ProductConfiguration &product, QObject *parent)
        : QObject(parent), d_ptr(new InstallationManagerPrivate) {
        Q_D(InstallationManager);
        d->q_ptr = this;
        d->product = product;
        d->product.bundledExecutablePath = normalizedPath(product.bundledExecutablePath);
        d->product.executableRelativePath = QDir::cleanPath(QDir::fromNativeSeparators(product.executableRelativePath));
        if (!QRegularExpression(QStringLiteral("^[a-z0-9]+(?:-[a-z0-9]+)*$")).match(product.id).hasMatch() || d->product.executableRelativePath == QStringLiteral(".") ||
            QDir::isAbsolutePath(d->product.executableRelativePath) || d->product.executableRelativePath.startsWith(QStringLiteral("../")) ||
            d->product.executableRelativePath == QStringLiteral("..") || d->product.executableRelativePath.contains(u':'))
            qFatal("Invalid installation product configuration");
        if (d->product.downloadedVersionsDirectory.isEmpty())
            d->product.downloadedVersionsDirectory = QDir(Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::RuntimeData)).filePath(QStringLiteral("installations/") + product.id + QStringLiteral("/versions"));
        d->product.downloadedVersionsDirectory = normalizedPath(d->product.downloadedVersionsDirectory);
        if (d->product.catalogUrl.isEmpty())
            d->product.catalogUrl = QUrl(QStringLiteral("https://catalogs.diffscope.org/v1/") + product.id + u'/');
        if (!d->product.catalogUrl.path().endsWith(u'/'))
            d->product.catalogUrl.setPath(d->product.catalogUrl.path() + u'/');
        d->versionTimer = new QTimer(this);
        d->versionTimer->setSingleShot(true);
        d->versionTimer->setInterval(qMax(1, product.versionTimeoutMs));
        connect(d->versionTimer, &QTimer::timeout, this, [d] { d->completeVersionQuery({}); });
        d->loadSettings();
    }

    InstallationManager::~InstallationManager() {
        shutdown();
    }

    ProductConfiguration InstallationManager::product() const {
        Q_D(const InstallationManager);
        return d->product;
    }

    InstallationSelection InstallationManager::selection() const {
        Q_D(const InstallationManager);
        return d->selection;
    }

    void InstallationManager::setSelection(const InstallationSelection &selection) {
        Q_D(InstallationManager);
        const auto normalized = normalizedSelection(selection);
        if (d->selection == normalized)
            return;
        d->selection = normalized;
        d->saveSettings();
        Q_EMIT selectionChanged();
        Q_EMIT installationsChanged();
    }

    QString InstallationManager::defaultInstallationId() const {
        Q_D(const InstallationManager);
        return QFileInfo(d->product.bundledExecutablePath).isFile() ? QStringLiteral("bundled") : QString{};
    }

    QVariantList InstallationManager::installations(const InstallationSelection &selection) const {
        Q_D(const InstallationManager);
        QVariantList result{QVariantMap{
            {QStringLiteral("id"), QString{}}, {QStringLiteral("name"), tr("No installation selected")}, {QStringLiteral("path"), QString{}},
        }};
        if (!defaultInstallationId().isEmpty()) {
            result.append(QVariantMap{
                {QStringLiteral("id"), QStringLiteral("bundled")}, {QStringLiteral("name"), tr("Built-in")}, {QStringLiteral("path"), d->product.bundledExecutablePath},
            });
        }
        const QList<QPair<QString, QString>> channels{
            {QStringLiteral("stable"), tr("Stable")}, {QStringLiteral("beta"), tr("Beta")}, {QStringLiteral("nightly"), tr("Nightly")},
        };
        for (const auto &directory : QDir(d->product.downloadedVersionsDirectory).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            const auto path = QDir(directory.absoluteFilePath()).filePath(d->product.executableRelativePath);
            if (directory.isSymLink() || !QFileInfo(path).isFile())
                continue;
            auto name = tr("Downloaded %1").arg(directory.fileName());
            for (const auto &[id, label] : channels) {
                if (directory.fileName().startsWith(id + u'-')) {
                    name = tr("%1 %2 (%3)").arg(d->product.displayName, directory.fileName().sliced(id.size() + 1), label);
                    break;
                }
            }
            result.append(QVariantMap{
                {QStringLiteral("id"), QStringLiteral("downloaded:") + directory.fileName()}, {QStringLiteral("name"), name}, {QStringLiteral("path"), normalizedPath(path)},
            });
        }
        for (const auto &value : selection.localInstallations) {
            const auto entry = value.toMap();
            const auto path = normalizedPath(entry.value(QStringLiteral("path")).toString());
            const auto version = d->versions.value(path);
            result.append(QVariantMap{
                {QStringLiteral("id"), entry.value(QStringLiteral("id"))},
                {QStringLiteral("name"), version.isEmpty() ? tr("%1 (version unknown)").arg(d->product.displayName) : tr("%1 %2").arg(d->product.displayName, version)},
                {QStringLiteral("path"), path},
            });
        }
        bool found = false;
        for (const auto &entry : std::as_const(result))
            found |= entry.toMap().value(QStringLiteral("id")).toString() == selection.id;
        if (!found) {
            result.prepend(QVariantMap{
                {QStringLiteral("id"), selection.id}, {QStringLiteral("name"), tr("Selected installation (unavailable)")}, {QStringLiteral("path"), QString{}},
            });
        }
        return result;
    }

    QString InstallationManager::executablePath() const {
        Q_D(const InstallationManager);
        return executablePath(d->selection);
    }

    QString InstallationManager::executablePath(const InstallationSelection &selection) const {
        Q_D(const InstallationManager);
        if (selection.id == QStringLiteral("bundled"))
            return defaultInstallationId().isEmpty() ? QString{} : d->product.bundledExecutablePath;
        if (selection.id.startsWith(QStringLiteral("downloaded:"))) {
            const auto directory = d->downloadedDirectory(selection.id);
            return directory.isEmpty() ? QString{} : normalizedPath(QDir(directory).filePath(d->product.executableRelativePath));
        }
        for (const auto &value : selection.localInstallations) {
            const auto entry = value.toMap();
            if (entry.value(QStringLiteral("id")).toString() == selection.id)
                return normalizedPath(entry.value(QStringLiteral("path")).toString());
        }
        return {};
    }

    QString InstallationManager::channel(const InstallationSelection &selection) const {
        Q_D(const InstallationManager);
        const auto directory = d->downloadedDirectory(selection.id);
        if (!directory.isEmpty()) {
            const auto name = QFileInfo(directory).fileName();
            for (const auto &channel : {QStringLiteral("stable"), QStringLiteral("beta"), QStringLiteral("nightly")}) {
                if (name.startsWith(channel + u'-'))
                    return channel;
            }
        }
        return {};
    }

    QString InstallationManager::version(const InstallationSelection &selection) const {
        Q_D(const InstallationManager);
        const auto channel = this->channel(selection);
        if (!channel.isEmpty())
            return QFileInfo(d->downloadedDirectory(selection.id)).fileName().sliced(channel.size() + 1);
        return d->versions.value(executablePath(selection));
    }

    void InstallationManager::fetchLatestVersions() {
        Q_D(InstallationManager);
        if (d->shutdown || d->updateCatalog)
            return;
        auto catalog = new DownloadCatalog(d->product.catalogUrl, d->product.variant, this);
        d->updateCatalog = catalog;
        connect(catalog, &DownloadCatalog::changed, this, [this, d, catalog] {
            if (catalog->busy() || d->updateCatalog != catalog)
                return;
            const auto versions = catalog->latestVersions();
            const auto error = catalog->errorMessage();
            d->updateCatalog = nullptr;
            catalog->deleteLater();
            Q_EMIT latestVersionsReady(versions, error);
        });
        catalog->start();
    }

    void InstallationManager::cancelLatestVersionRequest() {
        Q_D(InstallationManager);
        if (!d->updateCatalog)
            return;
        d->updateCatalog->cancel();
        d->updateCatalog->deleteLater();
        d->updateCatalog = nullptr;
    }

    bool InstallationManager::addLocalInstallation(InstallationSelection &selection, const QString &path, QString *errorMessage) const {
        const auto normalized = normalizedPath(path);
        if (!QFileInfo(normalized).isFile()) {
            if (errorMessage)
                *errorMessage = tr("The executable does not exist:\n%1").arg(normalized);
            return false;
        }
        for (const auto &value : installations(selection)) {
            const auto entry = value.toMap();
            if (entry.value(QStringLiteral("path")).toString() == normalized) {
                selection.id = entry.value(QStringLiteral("id")).toString();
                return true;
            }
        }
        selection.id = QStringLiteral("local:") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        selection.localInstallations.append(QVariantMap{{QStringLiteral("id"), selection.id}, {QStringLiteral("path"), normalized}});
        return true;
    }

    bool InstallationManager::removeLocalInstallation(InstallationSelection &selection) const {
        if (!selection.id.startsWith(QStringLiteral("local:")))
            return false;
        const auto count = selection.localInstallations.removeIf([&](const auto &value) { return value.toMap().value(QStringLiteral("id")).toString() == selection.id; });
        if (count == 0)
            return false;
        selection.id.clear();
        return true;
    }

    void InstallationManager::refreshVersions(const InstallationSelection &selection) {
        Q_D(InstallationManager);
        if (d->shutdown)
            return;
        d->pendingVersionPaths.clear();
        for (const auto &value : selection.localInstallations) {
            const auto path = normalizedPath(value.toMap().value(QStringLiteral("path")).toString());
            if (!path.isEmpty() && path != d->versionPath && !d->pendingVersionPaths.contains(path))
                d->pendingVersionPaths.append(path);
        }
        d->startNextVersionQuery();
    }

    DownloadResult InstallationManager::download(QWindow *parent) {
        Q_D(InstallationManager);
        if (d->shutdown || d->busy)
            return {};
        const auto notify = qScopeGuard([this] { Q_EMIT busyChanged(); });
        QScopedValueRollback guard(d->busy, true);
        Q_EMIT busyChanged();
        const auto result = Installer::download(d->product, parent);
        if (!result.id.isEmpty())
            Q_EMIT installationsChanged();
        return result;
    }

    bool InstallationManager::uninstall(const QString &id, QWindow *parent, const PrepareRemoval &prepare, const QString &preparationText) {
        Q_D(InstallationManager);
        if (d->shutdown || d->busy)
            return false;
        const auto directory = d->downloadedDirectory(id);
        const auto info = QFileInfo(directory);
        if (directory.isEmpty() || !info.isDir() || info.isSymLink())
            return false;
        const auto notify = qScopeGuard([this] { Q_EMIT busyChanged(); });
        QScopedValueRollback guard(d->busy, true);
        Q_EMIT busyChanged();
        auto name = id.sliced(11);
        for (const auto &value : installations(d->selection)) {
            const auto entry = value.toMap();
            if (entry.value(QStringLiteral("id")).toString() == id) {
                name = entry.value(QStringLiteral("name")).toString();
                break;
            }
        }
        if (!Installer::confirmUninstall(d->product, name, normalizedPath(directory), preparationText, parent))
            return false;
        QString error;
        d->cancelVersionProbes(directory);
        if (prepare && !prepare(directory, &error)) {
            if (error.isEmpty())
                error = tr("Could not prepare the installation for removal.");
        }
        if (!error.isEmpty()) {
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), parent, tr("Uninstallation Failed"), error);
            return false;
        }
        const bool clearCurrent = d->selection.id == id || pathIsWithin(directory, executablePath());
        QTemporaryDir removal(QDir(d->product.downloadedVersionsDirectory).filePath(QStringLiteral(".installation-remove-XXXXXX")));
        if (!removal.isValid() || !QDir().rename(directory, QDir(removal.path()).filePath(QStringLiteral("installation")))) {
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), parent, tr("Uninstallation Failed"), tr("Could not remove the installation:\n%1").arg(normalizedPath(directory)));
            return false;
        }
        if (clearCurrent) {
            auto selection = d->selection;
            selection.id = defaultInstallationId();
            setSelection(selection);
        }
        Q_EMIT installationRemoved(id);
        Q_EMIT installationsChanged();
        if (!removal.remove()) {
            removal.setAutoRemove(false);
            SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), parent, tr("Uninstallation Cleanup Failed"), tr("The version was uninstalled, but some files could not be deleted:\n%1").arg(normalizedPath(removal.path())));
        }
        return true;
    }

    bool InstallationManager::busy() const {
        Q_D(const InstallationManager);
        return d->busy;
    }

    void InstallationManager::shutdown() {
        Q_D(InstallationManager);
        if (d->shutdown)
            return;
        d->shutdown = true;
        cancelLatestVersionRequest();
        ++d->versionRequestSerial;
        d->pendingVersionPaths.clear();
        d->versionPath.clear();
        d->versionTimer->stop();
        if (auto cancel = std::exchange(d->cancelVersionProbe, {}))
            cancel();
    }

    QString InstallationManager::normalizedPath(const QString &path) {
        if (path.trimmed().isEmpty())
            return {};
        const QFileInfo info(QDir::fromNativeSeparators(path.trimmed()));
        const auto canonical = info.canonicalFilePath();
        return QDir::toNativeSeparators(canonical.isEmpty() ? QDir::cleanPath(info.absoluteFilePath()) : canonical);
    }

    bool InstallationManager::pathIsWithin(const QString &directory, const QString &path) {
        if (directory.isEmpty() || path.isEmpty())
            return false;
        const auto relative = QDir(directory).relativeFilePath(path);
        return !QDir::isAbsolutePath(relative) && relative != QStringLiteral("..") && !relative.startsWith(QStringLiteral("../"));
    }

    InstallationSelection InstallationManager::normalizedSelection(const InstallationSelection &selection) {
        auto result = selection;
        for (auto &value : result.localInstallations) {
            auto entry = value.toMap();
            entry.insert(QStringLiteral("path"), normalizedPath(entry.value(QStringLiteral("path")).toString()));
            value = entry;
        }
        return result;
    }

    void InstallationManagerPrivate::loadSettings() {
        Q_Q(InstallationManager);
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(InstallationManager::staticMetaObject.className());
        settings->beginGroup(product.id);
        selection.id = settings->value(QStringLiteral("selectedInstallationId"), q->defaultInstallationId()).toString();
        selection.localInstallations = settings->value(QStringLiteral("localInstallations")).toList();
        settings->endGroup();
        settings->endGroup();
        selection = InstallationManager::normalizedSelection(selection);
    }

    void InstallationManagerPrivate::saveSettings() const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(InstallationManager::staticMetaObject.className());
        settings->beginGroup(product.id);
        settings->setValue(QStringLiteral("selectedInstallationId"), selection.id);
        settings->setValue(QStringLiteral("localInstallations"), selection.localInstallations);
        settings->endGroup();
        settings->endGroup();
    }

    QString InstallationManagerPrivate::downloadedDirectory(const QString &id) const {
        if (!id.startsWith(QStringLiteral("downloaded:")))
            return {};
        const auto name = id.sliced(11);
        if (name.isEmpty() || name == QStringLiteral(".") || name == QStringLiteral("..") || name.contains(u'/') || name.contains(u'\\') || name.contains(u':'))
            return {};
        return QDir(product.downloadedVersionsDirectory).filePath(name);
    }

    void InstallationManagerPrivate::startNextVersionQuery() {
        Q_Q(InstallationManager);
        if (shutdown || !versionPath.isEmpty() || pendingVersionPaths.isEmpty())
            return;
        versionPath = pendingVersionPaths.takeFirst();
        const auto serial = ++versionRequestSerial;
        if (!product.probeVersion) {
            completeVersionQuery({});
            return;
        }
        versionTimer->start();
        const QPointer<InstallationManager> guard(q);
        cancelVersionProbe = product.probeVersion(versionPath, q, [guard, serial](const QString &version) {
            if (!guard)
                return;
            QMetaObject::invokeMethod(guard, [guard, serial, version] {
                if (guard && guard->d_func()->versionRequestSerial == serial)
                    guard->d_func()->completeVersionQuery(version);
            }, Qt::QueuedConnection);
        });
    }

    void InstallationManagerPrivate::completeVersionQuery(const QString &version) {
        Q_Q(InstallationManager);
        versionTimer->stop();
        ++versionRequestSerial;
        const auto path = std::exchange(versionPath, {});
        if (auto cancel = std::exchange(cancelVersionProbe, {}))
            cancel();
        if (path.isEmpty() || shutdown)
            return;
        if (!versions.contains(path) || versions.value(path) != version.trimmed()) {
            versions.insert(path, version.trimmed());
            Q_EMIT q->installationsChanged();
        }
        QTimer::singleShot(0, q, [this] { startNextVersionQuery(); });
    }

    void InstallationManagerPrivate::cancelVersionProbes(const QString &directory) {
        pendingVersionPaths.removeIf([&directory](const auto &path) { return InstallationManager::pathIsWithin(directory, path); });
        if (!InstallationManager::pathIsWithin(directory, versionPath))
            return;
        ++versionRequestSerial;
        versionPath.clear();
        versionTimer->stop();
        if (auto cancel = std::exchange(cancelVersionProbe, {}))
            cancel();
        Q_Q(InstallationManager);
        QTimer::singleShot(0, q, [this] { startNextVersionQuery(); });
    }

}

#include "moc_InstallationManager.cpp"
