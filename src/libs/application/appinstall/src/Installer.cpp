// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Installer_p.h"

#include <archive.h>
#include <archive_entry.h>
#include <array>
#include <functional>
#include <memory>

#ifndef Q_OS_WIN
#    include <unistd.h>
#endif

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QQmlComponent>
#include <QSaveFile>
#include <QSet>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTimer>
#include <QWindow>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/runtimeinterface.h>

#include <SVSCraftNetwork/DownloadSession.h>

#ifdef MessageBox
#    undef MessageBox
#endif

#include <SVSCraftQuick/MessageBox.h>

#include <appinstall/InstallationManager.h>
#include <appinstall/private/DownloadCatalog_p.h>

namespace AppInstall {

    namespace {

        using Cancelled = std::function<bool()>;

        bool copyExecutable(const QString &sourcePath, const QString &destination, const Cancelled &cancelled, QString *error) {
            QFile source(sourcePath);
            if (!source.open(QIODevice::ReadOnly)) {
                *error = source.errorString();
                return false;
            }
            const auto directory = QFileInfo(destination).absolutePath();
            if (!QDir().mkpath(directory)) {
                *error = Installer::tr("Could not create the directory:\n%1").arg(QDir::toNativeSeparators(directory));
                return false;
            }
            QSaveFile output(destination);
            if (!output.open(QIODevice::WriteOnly)) {
                *error = output.errorString();
                return false;
            }
#ifndef Q_OS_WIN
            if (!output.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther)) {
                *error = output.errorString();
                return false;
            }
#endif
            while (!source.atEnd()) {
                QCoreApplication::processEvents();
                if (cancelled())
                    return false;
                const auto data = source.read(1024 * 1024);
                if (source.error() != QFileDevice::NoError) {
                    *error = source.errorString();
                    return false;
                }
                if (output.write(data) != data.size()) {
                    *error = output.errorString();
                    return false;
                }
            }
            if (!output.commit()) {
                *error = output.errorString();
                return false;
            }
            return true;
        }

        bool extract(const QString &sourcePath, const QString &destination, const Cancelled &cancelled, QString *error) {
            const auto reject = [error](const QString &message) {
                *error = message;
                return false;
            };
            QFile source(sourcePath);
            if (!source.open(QIODevice::ReadOnly))
                return reject(source.errorString());
            std::unique_ptr<archive, decltype(&archive_read_free)> reader(archive_read_new(), archive_read_free);
            if (!reader)
                return reject(Installer::tr("Could not initialize archive extraction."));
            archive_read_support_filter_all(reader.get());
            archive_read_support_format_all(reader.get());
            const auto archiveError = [&] {
                const auto message = archive_error_string(reader.get());
                return reject(message ? QString::fromUtf8(message) : Installer::tr("Could not read the downloaded archive."));
            };
            if (archive_read_open_fd(reader.get(), source.handle(), 64 * 1024) != ARCHIVE_OK)
                return archiveError();
            struct Link {
                QString path;
                QString target;
            };
            QList<Link> links;
            QSet<QString> files;
            archive_entry *entry{};
            int status;
            while ((status = archive_read_next_header(reader.get(), &entry)) == ARCHIVE_OK) {
                QCoreApplication::processEvents();
                if (cancelled())
                    return false;
                const auto rawName = archive_entry_pathname_utf8(entry);
                const auto name = QString::fromUtf8(rawName ? rawName : archive_entry_pathname(entry)).replace(u'\\', u'/');
                const auto relative = QDir::cleanPath(name);
                const auto path = QDir(destination).absoluteFilePath(relative);
                if (name.isEmpty() || QDir::isAbsolutePath(relative) || name.contains(u':') || name.split(u'/').contains(QStringLiteral("..")) || !InstallationManager::pathIsWithin(destination, path))
                    return reject(Installer::tr("The archive contains an unsafe path: %1").arg(QDir::toNativeSeparators(name)));
                const auto type = archive_entry_filetype(entry);
                if (relative == QStringLiteral(".") && type == AE_IFDIR)
                    continue;
                if (archive_entry_hardlink(entry))
                    return reject(Installer::tr("The archive contains an unsupported hard link."));
                if (type == AE_IFDIR) {
                    if (!QDir().mkpath(path))
                        return reject(Installer::tr("Could not create the directory:\n%1").arg(QDir::toNativeSeparators(path)));
                    continue;
                }
                auto key = path;
#ifdef Q_OS_WIN
                key = key.toCaseFolded();
#endif
                if (files.contains(key) || QFileInfo::exists(path))
                    return reject(Installer::tr("The archive contains duplicate paths."));
                files.insert(key);
                if (!QDir().mkpath(QFileInfo(path).absolutePath()))
                    return reject(Installer::tr("Could not create the directory:\n%1").arg(QDir::toNativeSeparators(QFileInfo(path).absolutePath())));
                if (type == AE_IFLNK) {
                    const auto rawTarget = archive_entry_symlink_utf8(entry);
                    const auto target = QString::fromUtf8(rawTarget ? rawTarget : archive_entry_symlink(entry)).replace(u'\\', u'/');
                    if (target.isEmpty() || QDir::isAbsolutePath(target) || target.contains(u':') || !InstallationManager::pathIsWithin(destination, QDir(QFileInfo(path).absolutePath()).absoluteFilePath(target)))
                        return reject(Installer::tr("The archive contains an unsafe symbolic link."));
                    links.append({path, target});
                } else if (type == AE_IFREG) {
                    QSaveFile output(path);
                    if (!output.open(QIODevice::WriteOnly))
                        return reject(output.errorString());
                    std::array<char, 64 * 1024> buffer;
                    la_ssize_t count;
                    while ((count = archive_read_data(reader.get(), buffer.data(), buffer.size())) > 0) {
                        QCoreApplication::processEvents();
                        if (cancelled())
                            return false;
                        if (output.write(buffer.data(), count) != count)
                            return reject(output.errorString());
                    }
                    if (count < 0)
                        return archiveError();
                    if (!output.commit())
                        return reject(output.errorString());
#ifndef Q_OS_WIN
                    const auto mode = archive_entry_perm(entry);
                    QFileDevice::Permissions permissions;
                    if (mode & 0400)
                        permissions |= QFileDevice::ReadOwner;
                    if (mode & 0200)
                        permissions |= QFileDevice::WriteOwner;
                    if (mode & 0100)
                        permissions |= QFileDevice::ExeOwner;
                    if (mode & 0040)
                        permissions |= QFileDevice::ReadGroup;
                    if (mode & 0020)
                        permissions |= QFileDevice::WriteGroup;
                    if (mode & 0010)
                        permissions |= QFileDevice::ExeGroup;
                    if (mode & 0004)
                        permissions |= QFileDevice::ReadOther;
                    if (mode & 0002)
                        permissions |= QFileDevice::WriteOther;
                    if (mode & 0001)
                        permissions |= QFileDevice::ExeOther;
                    if (!QFile::setPermissions(path, permissions))
                        return reject(Installer::tr("Could not restore file permissions:\n%1").arg(QDir::toNativeSeparators(path)));
#endif
                } else {
                    return reject(Installer::tr("The archive contains an unsupported file type."));
                }
            }
            if (status != ARCHIVE_EOF)
                return archiveError();
            // Create links after regular files to prevent extraction through a symbolic link.
            for (const auto &link : links) {
#ifdef Q_OS_WIN
                return reject(Installer::tr("The archive contains an unsupported symbolic link."));
#else
                if (::symlink(QFile::encodeName(link.target).constData(), QFile::encodeName(link.path).constData()) != 0)
                    return reject(Installer::tr("Could not create the symbolic link:\n%1").arg(QDir::toNativeSeparators(link.path)));
#endif
            }
            return true;
        }

        void finishDialog(QObject *dialog, const QString &result) {
            QMetaObject::invokeMethod(dialog, "finish", Q_ARG(QVariant, result));
        }

    }

    DownloadResult Installer::download(const ProductConfiguration &product, QWindow *parent) {
        const auto &versionsDirectory = product.downloadedVersionsDirectory;
        DownloadCatalog catalog(product.catalogUrl, product.variant);
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.AppInstall"), QStringLiteral("DownloadDialog"));
        if (component.isError())
            qFatal() << component.errorString();
        auto object = component.createWithInitialProperties({
            {QStringLiteral("productName"), product.displayName},
            {QStringLiteral("catalog"), QVariant::fromValue(&catalog)},
            {QStringLiteral("transientParent"), QVariant::fromValue(parent)},
        });
        if (!object)
            qFatal() << component.errorString();
        std::unique_ptr<QObject> dialog(object);
        const auto cancelled = [&] { return dialog->property("completed").toBool(); };
        const auto reportError = [&](const QString &message) {
            finishDialog(dialog.get(), QStringLiteral("error"));
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), parent, tr("%1 Download Failed").arg(product.displayName), message);
            return DownloadResult{};
        };
        QTimer::singleShot(0, &catalog, &DownloadCatalog::start);
        if (SVS::MessageBox::customExec(dialog.get()).toString() != QStringLiteral("download")) {
            catalog.cancel();
            return {};
        }
        const auto artifact = catalog.artifact();
        catalog.cancel();
        if (artifact.url.isEmpty())
            return reportError(tr("The selected version is not available for this system."));
        const auto directoryName = artifact.channel + u'-' + artifact.version;
        const auto target = QDir(versionsDirectory).filePath(directoryName);
        const auto validate = [&](const QString &directory) {
            InstallationValidationResult result;
            result.success = true;
            if (!product.validateInstallation)
                return result;
            dialog->setProperty("phase", QStringLiteral("validate"));
            dialog->setProperty("statusText", tr("Validating %1...").arg(product.displayName));
            const InstallationCandidate candidate{
                QStringLiteral("downloaded:") + directoryName, directory, QDir(directory).filePath(product.executableRelativePath), artifact.channel, artifact.version,
            };
            return product.validateInstallation(candidate, {cancelled, [&](const QString &text) { dialog->setProperty("statusText", text); }});
        };
        const auto targetInfo = QFileInfo(target);
        if (targetInfo.isSymLink())
            return reportError(tr("The installation directory is a symbolic link:\n%1").arg(InstallationManager::normalizedPath(target)));
        if (QFileInfo::exists(QDir(target).filePath(product.executableRelativePath))) {
            const auto validation = validate(target);
            if (cancelled() || validation.cancelled)
                return {};
            if (!validation.success)
                return reportError(validation.errorMessage.isEmpty() ? tr("Installation validation failed.") : validation.errorMessage);
            finishDialog(dialog.get(), QStringLiteral("completed"));
            return {QStringLiteral("downloaded:") + directoryName, validation.data};
        }
        if (targetInfo.exists())
            return reportError(tr("The installation directory already exists:\n%1").arg(InstallationManager::normalizedPath(target)));
        if (!QDir().mkpath(versionsDirectory))
            return reportError(tr("Could not create the installation directory:\n%1").arg(versionsDirectory));
        const auto temporaryDirectory = Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::TempData);
        if (!QDir().mkpath(temporaryDirectory))
            return reportError(tr("Could not create the download directory:\n%1").arg(InstallationManager::normalizedPath(temporaryDirectory)));
        QTemporaryFile downloadedFile(QDir(temporaryDirectory).filePath(QStringLiteral("installation-download-XXXXXX")));
        if (!downloadedFile.open())
            return reportError(downloadedFile.errorString());
        dialog->setProperty("phase", QStringLiteral("download"));
        SVS::DownloadSession session(artifact.url, &downloadedFile);
        session.setUserAgent(QStringLiteral("DiffScope/%1").arg(QCoreApplication::applicationVersion()));
        QString downloadError;
        QObject::connect(&session, &SVS::DownloadSession::networkErrorOccurred, dialog.get(), [&](QNetworkReply::NetworkError, const QString &message) { downloadError = message; });
        QObject::connect(&session, &SVS::DownloadSession::writeErrorOccurred, dialog.get(), [&](const QString &message) { downloadError = message; });
        QObject::connect(&session, &SVS::DownloadSession::progressChanged, dialog.get(), [&](qint64 received, qint64 total) {
            dialog->setProperty("bytesReceived", received);
            dialog->setProperty("bytesTotal", total);
            dialog->setProperty("progress", total > 0 ? static_cast<double>(received) / total : 0.0);
            if (received > artifact.size) {
                downloadError = tr("The download exceeds the file size recorded in the catalog.");
                session.cancel();
            }
        });
        QObject::connect(&session, &SVS::DownloadSession::finished, dialog.get(), [&] {
            QMetaObject::invokeMethod(dialog.get(), "yieldResult", Q_ARG(QVariant, session.status() == SVS::DownloadSession::Status::Completed ? QStringLiteral("downloaded") : QStringLiteral("error")));
        }, Qt::QueuedConnection);
        QTimer::singleShot(0, &session, &SVS::DownloadSession::start);
        const auto result = SVS::MessageBox::customExec(dialog.get()).toString();
        if (cancelled()) {
            session.cancel();
            return {};
        }
        if (result != QStringLiteral("downloaded"))
            return reportError(downloadError.isEmpty() ? tr("%1 could not be downloaded.").arg(product.displayName) : downloadError);
        if (!downloadedFile.flush() || downloadedFile.size() != artifact.size)
            return reportError(tr("The downloaded file size does not match the catalog."));
        dialog->setProperty("phase", QStringLiteral("verify"));
        if (!downloadedFile.seek(0))
            return reportError(downloadedFile.errorString());
        QCryptographicHash hash(QCryptographicHash::Sha256);
        while (!downloadedFile.atEnd()) {
            QCoreApplication::processEvents();
            if (cancelled())
                return {};
            const auto data = downloadedFile.read(1024 * 1024);
            if (downloadedFile.error() != QFileDevice::NoError)
                return reportError(downloadedFile.errorString());
            hash.addData(data);
        }
        if (hash.result() != artifact.sha256)
            return reportError(tr("The downloaded file checksum does not match the catalog."));
        downloadedFile.close();
        QTemporaryDir stage(QDir(versionsDirectory).filePath(QStringLiteral(".installation-stage-XXXXXX")));
        if (!stage.isValid())
            return reportError(tr("Could not create a temporary installation directory."));
        dialog->setProperty("phase", QStringLiteral("install"));
        QString error;
        QString installation;
        if (artifact.format == QStringLiteral("exe") || artifact.format == QStringLiteral("binary")) {
            installation = stage.path();
            if (!copyExecutable(downloadedFile.fileName(), QDir(installation).filePath(product.executableRelativePath), cancelled, &error))
                return cancelled() ? DownloadResult{} : reportError(error);
        } else {
            if (!extract(downloadedFile.fileName(), stage.path(), cancelled, &error))
                return cancelled() ? DownloadResult{} : reportError(error);
            if (QFileInfo(QDir(stage.path()).filePath(product.executableRelativePath)).isFile()) {
                installation = stage.path();
            } else {
                for (const auto &entry : QDir(stage.path()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                    if (entry.isSymLink() || !QFileInfo(QDir(entry.absoluteFilePath()).filePath(product.executableRelativePath)).isFile())
                        continue;
                    if (!installation.isEmpty())
                        return reportError(tr("The archive contains multiple %1 installations.").arg(product.displayName));
                    installation = entry.absoluteFilePath();
                }
            }
            if (installation.isEmpty())
                return reportError(tr("The archive does not contain the %1 executable.").arg(product.displayName));
        }
        QDirIterator paths(installation, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
        while (paths.hasNext()) {
            paths.next();
            const auto info = paths.fileInfo();
            if (info.isSymLink() && !InstallationManager::pathIsWithin(installation, info.symLinkTarget()))
                return reportError(tr("The archive contains an unsafe symbolic link."));
        }
        const auto validation = validate(installation);
        if (cancelled() || validation.cancelled)
            return {};
        if (!validation.success)
            return reportError(validation.errorMessage.isEmpty() ? tr("Installation validation failed.") : validation.errorMessage);
        QCoreApplication::processEvents();
        if (cancelled())
            return {};
        if (!QDir().rename(installation, target))
            return reportError(tr("Could not move the installation into the data directory:\n%1").arg(InstallationManager::normalizedPath(target)));
        finishDialog(dialog.get(), QStringLiteral("completed"));
        return {QStringLiteral("downloaded:") + directoryName, validation.data};
    }

    bool Installer::confirmUninstall(const ProductConfiguration &product, const QString &name, const QString &directory, const QString &preparationText, QWindow *parent) {
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.AppInstall"), QStringLiteral("UninstallDialog"));
        if (component.isError())
            qFatal() << component.errorString();
        auto object = component.createWithInitialProperties({
            {QStringLiteral("productName"), product.displayName},
            {QStringLiteral("installationName"), name},
            {QStringLiteral("directory"), directory},
            {QStringLiteral("preparationText"), preparationText},
            {QStringLiteral("transientParent"), QVariant::fromValue(parent)},
        });
        if (!object)
            qFatal() << component.errorString();
        std::unique_ptr<QObject> dialog(object);
        return SVS::MessageBox::customExec(dialog.get()).toString() == QStringLiteral("uninstall");
    }

}

#include "moc_Installer_p.cpp"
