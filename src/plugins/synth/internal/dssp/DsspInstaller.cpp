// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DsspInstaller.h"

#include <array>
#include <functional>
#include <memory>

#ifndef Q_OS_WIN
#include <unistd.h>
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
#  undef MessageBox
#endif

#include <SVSCraftQuick/MessageBox.h>

#include <synth/internal/DsspDownloadCatalog.h>
#include <synth/internal/DsspServerManager.h>

#include <archive.h>
#include <archive_entry.h>

#ifdef MessageBox
#  undef MessageBox
#endif

namespace Synth::Internal {

    namespace {

        using Cancelled = std::function<bool()>;

        QString executableRelativePath() {
#ifdef Q_OS_WIN
            return QStringLiteral("bin/dssp.exe");
#else
            return QStringLiteral("bin/dssp");
#endif
        }

        bool isInside(const QString &directory, const QString &path) {
            const auto relative = QDir(directory).relativeFilePath(path);
            return !QDir::isAbsolutePath(relative) && relative != QStringLiteral("..") && !relative.startsWith(QStringLiteral("../"));
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
                return reject(DsspInstaller::tr("Could not initialize archive extraction."));
            archive_read_support_filter_all(reader.get());
            archive_read_support_format_all(reader.get());
            const auto archiveError = [&] {
                const auto message = archive_error_string(reader.get());
                return reject(message ? QString::fromUtf8(message) : DsspInstaller::tr("Could not read the downloaded archive."));
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
                if (name.isEmpty() || QDir::isAbsolutePath(relative) || name.contains(u':') || name.split(u'/').contains(QStringLiteral("..")) || !isInside(destination, path))
                    return reject(DsspInstaller::tr("The archive contains an unsafe path: %1").arg(QDir::toNativeSeparators(name)));
                const auto type = archive_entry_filetype(entry);
                if (relative == QStringLiteral(".") && type == AE_IFDIR)
                    continue;
                if (archive_entry_hardlink(entry))
                    return reject(DsspInstaller::tr("The archive contains an unsupported hard link."));
                if (type == AE_IFDIR) {
                    if (!QDir().mkpath(path))
                        return reject(DsspInstaller::tr("Could not create the directory:\n%1").arg(QDir::toNativeSeparators(path)));
                    continue;
                }
                auto key = path;
#ifdef Q_OS_WIN
                key = key.toCaseFolded();
#endif
                if (files.contains(key) || QFileInfo::exists(path))
                    return reject(DsspInstaller::tr("The archive contains duplicate paths."));
                files.insert(key);
                if (!QDir().mkpath(QFileInfo(path).absolutePath()))
                    return reject(DsspInstaller::tr("Could not create the directory:\n%1").arg(QDir::toNativeSeparators(QFileInfo(path).absolutePath())));
                if (type == AE_IFLNK) {
                    const auto rawTarget = archive_entry_symlink_utf8(entry);
                    const auto target = QString::fromUtf8(rawTarget ? rawTarget : archive_entry_symlink(entry)).replace(u'\\', u'/');
                    if (target.isEmpty() || QDir::isAbsolutePath(target) || target.contains(u':') || !isInside(destination, QDir(QFileInfo(path).absolutePath()).absoluteFilePath(target)))
                        return reject(DsspInstaller::tr("The archive contains an unsafe symbolic link."));
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
                        return reject(DsspInstaller::tr("Could not restore file permissions:\n%1").arg(QDir::toNativeSeparators(path)));
#endif
                } else {
                    return reject(DsspInstaller::tr("The archive contains an unsupported file type."));
                }
            }
            if (status != ARCHIVE_EOF)
                return archiveError();
            // Create links after regular files to prevent extraction through a symbolic link.
            for (const auto &link : links) {
#ifdef Q_OS_WIN
                return reject(DsspInstaller::tr("The archive contains an unsupported symbolic link."));
#else
                if (::symlink(QFile::encodeName(link.target).constData(), QFile::encodeName(link.path).constData()) != 0)
                    return reject(DsspInstaller::tr("Could not create the symbolic link:\n%1").arg(QDir::toNativeSeparators(link.path)));
#endif
            }
            return true;
        }

        void finishDialog(QObject *dialog, const QString &result) {
            QMetaObject::invokeMethod(dialog, "finish", Q_ARG(QVariant, result));
        }

    }

    QString DsspInstaller::download(const QString &versionsDirectory, QWindow *parent) {
        DsspDownloadCatalog catalog;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("DsspDownloadDialog"));
        if (component.isError())
            qFatal() << component.errorString();
        auto object = component.createWithInitialProperties({
            {QStringLiteral("catalog"), QVariant::fromValue(&catalog)},
            {QStringLiteral("transientParent"), QVariant::fromValue(parent)},
        });
        if (!object)
            qFatal() << component.errorString();
        std::unique_ptr<QObject> dialog(object);
        const auto cancelled = [&] { return dialog->property("completed").toBool(); };
        const auto reportError = [&](const QString &message) {
            finishDialog(dialog.get(), QStringLiteral("error"));
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), parent, tr("DiffSinger Synthesis Program Download Failed"), message);
            return QString{};
        };
        QTimer::singleShot(0, &catalog, &DsspDownloadCatalog::start);
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
        const auto targetInfo = QFileInfo(target);
        if (targetInfo.isSymLink())
            return reportError(tr("The installation directory is a symbolic link:\n%1").arg(DsspServerManager::normalizedPath(target)));
        if (QFileInfo::exists(QDir(target).filePath(executableRelativePath()))) {
            finishDialog(dialog.get(), QStringLiteral("completed"));
            return QStringLiteral("downloaded:") + directoryName;
        }
        if (targetInfo.exists())
            return reportError(tr("The installation directory already exists:\n%1").arg(DsspServerManager::normalizedPath(target)));
        if (!QDir().mkpath(versionsDirectory))
            return reportError(tr("Could not create the installation directory:\n%1").arg(versionsDirectory));
        const auto temporaryDirectory = Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::TempData);
        if (!QDir().mkpath(temporaryDirectory))
            return reportError(tr("Could not create the download directory:\n%1").arg(DsspServerManager::normalizedPath(temporaryDirectory)));
        QTemporaryFile archiveFile(QDir(temporaryDirectory).filePath(QStringLiteral("dssp-download-XXXXXX")));
        if (!archiveFile.open())
            return reportError(archiveFile.errorString());
        dialog->setProperty("phase", QStringLiteral("download"));
        SVS::DownloadSession session(artifact.url, &archiveFile);
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
            return reportError(downloadError.isEmpty() ? tr("DiffSinger Synthesis Program could not be downloaded.") : downloadError);
        if (!archiveFile.flush() || archiveFile.size() != artifact.size)
            return reportError(tr("The downloaded file size does not match the catalog."));
        dialog->setProperty("phase", QStringLiteral("verify"));
        if (!archiveFile.seek(0))
            return reportError(archiveFile.errorString());
        QCryptographicHash hash(QCryptographicHash::Sha256);
        while (!archiveFile.atEnd()) {
            QCoreApplication::processEvents();
            if (cancelled())
                return {};
            const auto data = archiveFile.read(1024 * 1024);
            if (archiveFile.error() != QFileDevice::NoError)
                return reportError(archiveFile.errorString());
            hash.addData(data);
        }
        if (hash.result() != artifact.sha256)
            return reportError(tr("The downloaded file checksum does not match the catalog."));
        archiveFile.close();
        QTemporaryDir stage(QDir(versionsDirectory).filePath(QStringLiteral(".dssp-install-XXXXXX")));
        if (!stage.isValid())
            return reportError(tr("Could not create a temporary installation directory."));
        dialog->setProperty("phase", QStringLiteral("install"));
        QString error;
        if (!extract(archiveFile.fileName(), stage.path(), cancelled, &error))
            return cancelled() ? QString{} : reportError(error);
        QString installation;
        if (QFileInfo(QDir(stage.path()).filePath(executableRelativePath())).isFile()) {
            installation = stage.path();
        } else {
            for (const auto &entry : QDir(stage.path()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (entry.isSymLink() || !QFileInfo(QDir(entry.absoluteFilePath()).filePath(executableRelativePath())).isFile())
                    continue;
                if (!installation.isEmpty())
                    return reportError(tr("The archive contains multiple DiffSinger Synthesis Program installations."));
                installation = entry.absoluteFilePath();
            }
        }
        if (installation.isEmpty())
            return reportError(tr("The archive does not contain the DiffSinger Synthesis Program executable."));
        QDirIterator paths(installation, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
        while (paths.hasNext()) {
            paths.next();
            const auto info = paths.fileInfo();
            if (info.isSymLink() && !isInside(installation, info.symLinkTarget()))
                return reportError(tr("The archive contains an unsafe symbolic link."));
        }
        QCoreApplication::processEvents();
        if (cancelled())
            return {};
        if (!QDir().rename(installation, target))
            return reportError(tr("Could not move the installation into the data directory:\n%1").arg(DsspServerManager::normalizedPath(target)));
        finishDialog(dialog.get(), QStringLiteral("completed"));
        return QStringLiteral("downloaded:") + directoryName;
    }

}

#include "moc_DsspInstaller.cpp"
