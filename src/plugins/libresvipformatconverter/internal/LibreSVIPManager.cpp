// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#include "LibreSVIPManager.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>

#include <QBuffer>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QLocale>
#include <QNetworkReply>
#include <QPointer>
#include <QProcess>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QSaveFile>
#include <QSettings>
#include <QTemporaryFile>
#include <QThread>
#include <QTimer>
#include <QSysInfo>
#include <QUuid>
#include <QVariant>

#include <QtGrpc/QGrpcCallOptions>
#include <QtGrpc/QGrpcCallReply>
#include <QtGrpc/QGrpcHttp2Channel>

#include <archive.h>
#include <archive_entry.h>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/runtimeinterface.h>
#include <CoreApi/windowsystem.h>

#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftCore/Semver.h>
#include <SVSCraftNetwork/DownloadSession.h>

#ifdef MessageBox
#  undef MessageBox
#endif

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/NotificationMessage.h>

#include "libresvip.qpb.h"
#include "libresvip_client.grpc.qpb.h"

#ifdef MessageBox
#  undef MessageBox
#endif

using namespace std::chrono_literals;

namespace LibreSVIPFormatConverter::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcLibreSVIPManager, "diffscope.libresvipformatconverter.manager")

    static LibreSVIPManager *s_instance{};
    static constexpr qint64 kMaximumInputSize = 512ll * 1024ll * 1024ll;
    static constexpr int kCatalogTimeoutMs = 15000;
    static constexpr int kConversionTimeoutMs = 10 * 60 * 1000;
    static constexpr int kCacheVersion = 1;
    static constexpr auto kSettingsGroup = "LibreSVIPFormatConverter::Internal::LibreSVIPManager";
    static constexpr auto kDownloadedVersionFileName = ".diffscope-version";
    static const QUrl kGrpcEndpoint(QStringLiteral("http://127.0.0.1:15150"));
    using CancellationCheck = std::function<bool()>;

    struct LibreSVIPDownloadArtifact {
        SVS::Semver version;
        QString versionText;
        QUrl url;
    };

    struct LibreSVIPDownloadCatalogResult {
        QList<LibreSVIPDownloadArtifact> artifacts;
        QString errorMessage;
    };

    static QString currentArtifactKey() {
#if defined(Q_OS_WIN)
        const QString platform = QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
        const QString platform = QStringLiteral("macos");
#elif defined(Q_OS_LINUX)
        const QString platform = QStringLiteral("linux");
#else
        return {};
#endif

        const QString architecture = QSysInfo::currentCpuArchitecture().toLower();
        if (architecture == QStringLiteral("x86_64") || architecture == QStringLiteral("amd64") ||
            architecture == QStringLiteral("x64"))
            return platform + QStringLiteral("_amd64");
        if (architecture == QStringLiteral("arm64") || architecture == QStringLiteral("aarch64"))
            return platform + QStringLiteral("_arm64");
        return {};
    }

    static LibreSVIPDownloadCatalogResult parseDownloadCatalog(const QByteArray &data) {
        LibreSVIPDownloadCatalogResult result;
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
            const QString detail = parseError.error != QJsonParseError::NoError
                                       ? parseError.errorString()
                                       : QCoreApplication::translate(
                                             "LibreSVIPManager", "The root value is not an array.");
            result.errorMessage = QCoreApplication::translate(
                                      "LibreSVIPManager", "The LibreSVIP download catalog is invalid: %1")
                                      .arg(detail);
            return result;
        }

        const QString artifactKey = currentArtifactKey();
        for (const auto &value : document.array()) {
            if (!value.isObject())
                continue;
            const auto object = value.toObject();
            const QString versionText = object.value(QStringLiteral("version")).toString();
            const SVS::Semver version(versionText);
            const QString artifactUrl = object.value(QStringLiteral("artifacts"))
                                            .toObject()
                                            .value(artifactKey)
                                            .toString();
            const QUrl url(artifactUrl);
            if (version.isValid() && !artifactUrl.isEmpty() && url.isValid())
                result.artifacts.append({version, versionText, url});
        }
        std::sort(result.artifacts.begin(), result.artifacts.end(), [](const auto &left, const auto &right) {
            return left.version > right.version;
        });
        if (result.artifacts.isEmpty()) {
            result.errorMessage = QCoreApplication::translate(
                "LibreSVIPManager", "No LibreSVIP downloads are available for this system architecture.");
        }
        return result;
    }

    static QString compactJson(const QJsonObject &object) {
        return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
    }

    static bool cancellationRequested(const CancellationCheck &isCancelled) {
        return isCancelled && isCancelled();
    }

    static QByteArray sha512ForFile(const QString &path, const CancellationCheck &isCancelled = {},
                                    bool *cancelled = nullptr) {
        if (cancelled)
            *cancelled = false;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        QCryptographicHash hash(QCryptographicHash::Sha512);
        while (!file.atEnd()) {
            if (isCancelled) {
                QCoreApplication::processEvents();
                if (cancellationRequested(isCancelled)) {
                    if (cancelled)
                        *cancelled = true;
                    return {};
                }
            }
            const QByteArray block = file.read(1024 * 1024);
            if (block.isEmpty() && file.error() != QFileDevice::NoError)
                return {};
            hash.addData(block);
        }
        return hash.result();
    }

    static QString canonicalNativePath(const QString &path) {
        if (path.isEmpty())
            return {};
        const QString canonicalPath = QFileInfo(QDir::fromNativeSeparators(path)).canonicalFilePath();
        return QDir::toNativeSeparators(canonicalPath);
    }

    static QString absoluteNativePath(const QString &path) {
        if (path.isEmpty())
            return {};
        const QString absolutePath = QFileInfo(QDir::fromNativeSeparators(path)).absoluteFilePath();
        return QDir::toNativeSeparators(QDir::cleanPath(absolutePath));
    }

    static QString userFacingRpcError(const QString &error) {
        const auto lines = error.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (auto it = lines.crbegin(); it != lines.crend(); ++it) {
            const auto line = it->trimmed();
            if (!line.isEmpty() && !line.startsWith(QStringLiteral("Traceback")))
                return line;
        }
        return error.trimmed();
    }

    static bool pathIsWithin(const QString &rootPath, const QString &candidatePath) {
        QString root = QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(rootPath).absoluteFilePath()));
        QString candidate = QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(candidatePath).absoluteFilePath()));
#ifdef Q_OS_WINDOWS
        root = root.toLower();
        candidate = candidate.toLower();
#endif
        if (candidate == root)
            return true;
        return candidate.startsWith(root + QLatin1Char('/'));
    }

    static QObject *createQuickObject(const char *componentName, const QVariantMap &properties) {
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.LibreSVIPFormatConverter", componentName);
        if (component.isError()) {
            qCCritical(lcLibreSVIPManager) << component.errorString();
            return nullptr;
        }
        auto object = component.createWithInitialProperties(properties);
        if (!object)
            qCCritical(lcLibreSVIPManager) << component.errorString();
        return object;
    }

    static void finishQuickDialog(QObject *dialog, const QVariant &result) {
        if (dialog)
            QMetaObject::invokeMethod(dialog, "finish", Q_ARG(QVariant, result));
    }

    static void yieldQuickDialog(QObject *dialog, const QVariant &result) {
        if (dialog)
            QMetaObject::invokeMethod(dialog, "yieldResult", Q_ARG(QVariant, result));
    }

    static bool quickDialogWasCancelled(const QObject *dialog) {
        return dialog && dialog->property("finalResult").toString() == QStringLiteral("cancelled");
    }

    static bool waitForProcessStarted(QProcess *process, int timeoutMs, const CancellationCheck &isCancelled,
                                      bool *cancelled = nullptr) {
        if (cancelled)
            *cancelled = false;
        QElapsedTimer timer;
        timer.start();
        while (process->state() == QProcess::Starting && timer.elapsed() < timeoutMs) {
            QCoreApplication::processEvents();
            if (cancellationRequested(isCancelled)) {
                if (cancelled)
                    *cancelled = true;
                return false;
            }
            process->waitForStarted(50);
        }
        QCoreApplication::processEvents();
        if (cancellationRequested(isCancelled)) {
            if (cancelled)
                *cancelled = true;
            return false;
        }
        return process->state() == QProcess::Running;
    }

    static bool waitForReply(std::unique_ptr<QGrpcCallReply> &reply, QProtobufMessage *response,
                             QString *errorMessage, int hardTimeoutMs, const CancellationCheck &isCancelled = {},
                             bool *cancelled = nullptr) {
        if (cancelled)
            *cancelled = false;
        if (cancellationRequested(isCancelled)) {
            if (cancelled)
                *cancelled = true;
            reply->cancel();
            return false;
        }
        QEventLoop loop;
        QTimer timer;
        QTimer cancellationTimer;
        timer.setSingleShot(true);
        cancellationTimer.setInterval(50);
        bool succeeded = false;
        QObject::connect(reply.get(), &QGrpcCallReply::finished, &loop, [&](const QGrpcStatus &status) {
            if (status.isOk()) {
                succeeded = reply->read(response);
                if (!succeeded && errorMessage)
                    *errorMessage = QCoreApplication::translate("LibreSVIPManager", "Failed to decode the LibreSVIP RPC response.");
            } else if (errorMessage) {
                *errorMessage = status.message();
            }
            loop.quit();
        });
        QObject::connect(&timer, &QTimer::timeout, &loop, [&] {
            if (errorMessage)
                *errorMessage = QCoreApplication::translate("LibreSVIPManager", "The LibreSVIP RPC request timed out.");
            reply->cancel();
            loop.quit();
        });
        if (isCancelled) {
            QObject::connect(&cancellationTimer, &QTimer::timeout, &loop, [&] {
                if (!cancellationRequested(isCancelled))
                    return;
                if (cancelled)
                    *cancelled = true;
                reply->cancel();
                loop.quit();
            });
            cancellationTimer.start();
        }
        timer.start(hardTimeoutMs);
        loop.exec();
        return succeeded;
    }

    static LibreSVIPPluginInfo pluginInfoFromProto(const LibreSVIP::PluginInfo &source) {
        LibreSVIPPluginInfo target;
        target.identifier = source.identifier();
        target.name = source.name();
        target.version = source.version();
        target.description = source.description();
        target.author = source.author();
        target.website = source.website();
        target.fileFormat = source.fileFormat();
        target.suffixes = source.suffixes();
        target.iconBase64 = source.iconBase64();
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(source.jsonSchema().toUtf8(), &error);
        if (error.error == QJsonParseError::NoError && document.isObject())
            target.jsonSchema = document.object();
        else
            qCWarning(lcLibreSVIPManager) << "Invalid JSON schema from plugin" << target.identifier << error.errorString();
        return target;
    }

    static bool queryPluginInfos(LibreSVIP::Conversion::Client &client,
                                 LibreSVIP::PluginCategoryGadget::PluginCategory category,
                                 QList<LibreSVIPPluginInfo> *plugins, QString *errorMessage,
                                 int timeoutMs, const CancellationCheck &isCancelled = {},
                                 bool *cancelled = nullptr) {
        LibreSVIP::PluginInfosRequest request;
        request.setCategory(category);
        request.setLanguage(QLocale().name());
        QGrpcCallOptions options;
        options.setDeadlineTimeout(std::chrono::milliseconds(timeoutMs));
        auto reply = client.PluginInfos(request, options);
        LibreSVIP::PluginInfosResponse response;
        if (!waitForReply(reply, &response, errorMessage, timeoutMs + 250, isCancelled, cancelled))
            return false;
        plugins->clear();
        for (const auto &value : response.values())
            plugins->append(pluginInfoFromProto(value));
        return true;
    }

    static bool extractArchive(const QString &archivePath, const QString &destination, QString *errorMessage,
                               const CancellationCheck &isCancelled = {}, bool *cancelled = nullptr) {
        if (cancelled)
            *cancelled = false;
        std::unique_ptr<archive, decltype(&archive_read_free)> reader(archive_read_new(), archive_read_free);
        if (!reader) {
            *errorMessage = QCoreApplication::translate("LibreSVIPManager", "Failed to initialize the archive reader.");
            return false;
        }
        archive_read_support_filter_all(reader.get());
        archive_read_support_format_all(reader.get());
        const auto encodedPath = QFile::encodeName(archivePath);
        if (archive_read_open_filename(reader.get(), encodedPath.constData(), 10240) != ARCHIVE_OK) {
            *errorMessage = QString::fromLocal8Bit(archive_error_string(reader.get()));
            return false;
        }

        if (!QDir().mkpath(destination)) {
            *errorMessage = QCoreApplication::translate("LibreSVIPManager", "Failed to create the installation directory.");
            return false;
        }
        archive_entry *entry{};
        int archiveStatus = ARCHIVE_OK;
        while ((archiveStatus = archive_read_next_header(reader.get(), &entry)) == ARCHIVE_OK) {
            QCoreApplication::processEvents();
            if (cancellationRequested(isCancelled)) {
                if (cancelled)
                    *cancelled = true;
                return false;
            }
            const char *rawName = archive_entry_pathname_utf8(entry);
            if (!rawName)
                rawName = archive_entry_pathname(entry);
            QString rawRelative = QString::fromUtf8(rawName ? rawName : "");
            rawRelative.replace(QLatin1Char('\\'), QLatin1Char('/'));
            const QStringList pathParts = rawRelative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
            const QString relative = QDir::cleanPath(rawRelative);
            if (relative.isEmpty() || relative == QStringLiteral(".") || QDir::isAbsolutePath(relative) ||
                relative == QStringLiteral("..") || relative.startsWith(QStringLiteral("../")) ||
                pathParts.contains(QStringLiteral("..")) || relative.contains(QLatin1Char(':'))) {
                *errorMessage = QCoreApplication::translate("LibreSVIPManager", "The archive contains an unsafe path: %1")
                                    .arg(QDir::toNativeSeparators(relative));
                return false;
            }
            const QString target = QDir(destination).absoluteFilePath(relative);
            if (!pathIsWithin(destination, target)) {
                *errorMessage = QCoreApplication::translate("LibreSVIPManager", "The archive attempts to write outside the installation directory.");
                return false;
            }

            const auto fileType = archive_entry_filetype(entry);
            if (fileType == AE_IFDIR) {
                if (!QDir().mkpath(target)) {
                    *errorMessage = QCoreApplication::translate("LibreSVIPManager", "Failed to create directory: %1")
                                        .arg(QDir::toNativeSeparators(target));
                    return false;
                }
            } else if (fileType == AE_IFREG) {
                if (archive_entry_hardlink(entry)) {
                    *errorMessage = QCoreApplication::translate("LibreSVIPManager", "The archive contains an unsupported hard link.");
                    return false;
                }
                if (!QDir().mkpath(QFileInfo(target).absolutePath())) {
                    *errorMessage = QCoreApplication::translate("LibreSVIPManager", "Failed to create directory: %1")
                                        .arg(QDir::toNativeSeparators(QFileInfo(target).absolutePath()));
                    return false;
                }
                QSaveFile output(target);
                if (!output.open(QIODevice::WriteOnly)) {
                    *errorMessage = output.errorString();
                    return false;
                }
                char buffer[64 * 1024];
                la_ssize_t count;
                while ((count = archive_read_data(reader.get(), buffer, sizeof(buffer))) > 0) {
                    QCoreApplication::processEvents();
                    if (cancellationRequested(isCancelled)) {
                        if (cancelled)
                            *cancelled = true;
                        return false;
                    }
                    if (output.write(buffer, count) != count) {
                        *errorMessage = output.errorString();
                        return false;
                    }
                }
                if (count < 0 || !output.commit()) {
                    *errorMessage = count < 0 ? QString::fromLocal8Bit(archive_error_string(reader.get())) : output.errorString();
                    return false;
                }
                const auto mode = archive_entry_perm(entry);
                QFileDevice::Permissions permissions;
                if (mode & 0400) permissions |= QFileDevice::ReadOwner;
                if (mode & 0200) permissions |= QFileDevice::WriteOwner;
                if (mode & 0100) permissions |= QFileDevice::ExeOwner;
                if (mode & 0040) permissions |= QFileDevice::ReadGroup;
                if (mode & 0020) permissions |= QFileDevice::WriteGroup;
                if (mode & 0010) permissions |= QFileDevice::ExeGroup;
                if (mode & 0004) permissions |= QFileDevice::ReadOther;
                if (mode & 0002) permissions |= QFileDevice::WriteOther;
                if (mode & 0001) permissions |= QFileDevice::ExeOther;
                if (permissions != QFileDevice::Permissions{})
                    QFile::setPermissions(target, permissions);
            } else if (fileType == AE_IFLNK) {
                const char *rawLink = archive_entry_symlink_utf8(entry);
                if (!rawLink)
                    rawLink = archive_entry_symlink(entry);
                QString linkTarget = QString::fromUtf8(rawLink ? rawLink : "");
                linkTarget.replace(QLatin1Char('\\'), QLatin1Char('/'));
                const QString resolved = QDir(QFileInfo(target).absolutePath()).absoluteFilePath(linkTarget);
                if (linkTarget.isEmpty() || QDir::isAbsolutePath(linkTarget) || linkTarget.contains(QLatin1Char(':')) ||
                    !pathIsWithin(destination, resolved)) {
                    *errorMessage = QCoreApplication::translate("LibreSVIPManager", "The archive contains an unsafe symbolic link.");
                    return false;
                }
                QDir().mkpath(QFileInfo(target).absolutePath());
                if (!QFile::link(linkTarget, target)) {
                    *errorMessage = QCoreApplication::translate("LibreSVIPManager", "Failed to create symbolic link: %1")
                                        .arg(QDir::toNativeSeparators(target));
                    return false;
                }
            } else {
                archive_read_data_skip(reader.get());
            }
        }
        if (archiveStatus != ARCHIVE_EOF) {
            *errorMessage = QString::fromLocal8Bit(archive_error_string(reader.get()));
            return false;
        }
        return true;
    }

    LibreSVIPManager::LibreSVIPManager(QObject *parent) : QObject(parent) {
        Q_ASSERT(!s_instance);
        s_instance = this;
        loadConfiguration();
    }

    LibreSVIPManager::~LibreSVIPManager() {
        cancelUpdateCheck();
        dismissUpdateNotification();
        stopProcess(true);
        s_instance = nullptr;
    }

    LibreSVIPManager *LibreSVIPManager::instance() {
        return s_instance;
    }

    QString LibreSVIPManager::executablePath() const {
        return QDir::toNativeSeparators(m_executablePath);
    }

    const LibreSVIPPluginCatalog &LibreSVIPManager::catalog() const {
        return m_catalog;
    }

    bool LibreSVIPManager::hasCurrentCatalog() const {
        return cacheIsCurrent();
    }

    bool LibreSVIPManager::downloadedInstallationExists() const {
        return QDir(downloadedRoot()).exists();
    }

    bool LibreSVIPManager::autoCheckForUpdates() const {
        return m_autoCheckForUpdates;
    }

    void LibreSVIPManager::setAutoCheckForUpdates(bool enabled) {
        if (m_autoCheckForUpdates == enabled)
            return;
        m_autoCheckForUpdates = enabled;
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QString::fromLatin1(kSettingsGroup));
        settings->setValue(QStringLiteral("autoCheckForUpdates"), enabled);
        settings->endGroup();
        settings->sync();
        if (!enabled) {
            cancelUpdateCheck();
            dismissUpdateNotification();
        }
        Q_EMIT autoCheckForUpdatesChanged(enabled);
    }

    void LibreSVIPManager::loadConfiguration() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QString::fromLatin1(kSettingsGroup));
        m_autoCheckForUpdates = settings->value(QStringLiteral("autoCheckForUpdates"), true).toBool();
        bool configurationUpdated = false;
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(settings->value(QStringLiteral("configuration")).toByteArray(), &error);
        if (error.error == QJsonParseError::NoError && document.isObject()) {
            auto configuration = document.object();
            const QString storedPath = configuration.value(QStringLiteral("executablePath")).toString();
            m_executablePath = canonicalNativePath(storedPath);
            if (m_executablePath.isEmpty())
                m_executablePath = absoluteNativePath(storedPath);
            if (m_executablePath != storedPath) {
                configuration.insert(QStringLiteral("executablePath"), m_executablePath);
                settings->setValue(QStringLiteral("configuration"),
                                   QJsonDocument(configuration).toJson(QJsonDocument::Compact));
                configurationUpdated = true;
            }
            if (configuration.value(QStringLiteral("cacheVersion")).toInt() == kCacheVersion) {
                m_cachedSha512 = QByteArray::fromHex(configuration.value(QStringLiteral("sha512")).toString().toLatin1());
                m_cachedLocaleName = configuration.value(QStringLiteral("localeName")).toString();
                m_catalog = LibreSVIPPluginCatalog::fromJson(configuration.value(QStringLiteral("pluginCatalog")).toObject());
            }
        }
        settings->endGroup();
        if (configurationUpdated)
            settings->sync();
    }

    bool LibreSVIPManager::saveConfiguration(const QString &path, const LibreSVIPValidationResult &result) {
        const QString canonicalPath = canonicalNativePath(path);
        if (canonicalPath.isEmpty()) {
            qCCritical(lcLibreSVIPManager) << "Failed to resolve the canonical LibreSVIP executable path:" << path;
            return false;
        }
        m_executablePath = canonicalPath;
        m_cachedSha512 = result.sha512;
        m_cachedLocaleName = QLocale().name();
        m_catalog = result.catalog;
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QString::fromLatin1(kSettingsGroup));
        const QJsonObject configuration{
            {QStringLiteral("cacheVersion"), kCacheVersion},
            {QStringLiteral("executablePath"), m_executablePath},
            {QStringLiteral("sha512"), QString::fromLatin1(m_cachedSha512.toHex())},
            {QStringLiteral("localeName"), m_cachedLocaleName},
            {QStringLiteral("pluginCatalog"), m_catalog.toJson()},
        };
        settings->setValue(QStringLiteral("configuration"), QJsonDocument(configuration).toJson(QJsonDocument::Compact));
        settings->endGroup();
        settings->sync();
        Q_EMIT configurationChanged();
        Q_EMIT catalogChanged();
        return true;
    }

    void LibreSVIPManager::clearConfiguration() {
        m_executablePath.clear();
        m_cachedSha512.clear();
        m_cachedLocaleName.clear();
        m_catalog = {};
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QString::fromLatin1(kSettingsGroup));
        settings->remove(QStringLiteral("configuration"));
        settings->endGroup();
        settings->sync();
        Q_EMIT configurationChanged();
        Q_EMIT catalogChanged();
    }

    bool LibreSVIPManager::cacheIsCurrent() const {
        const QFileInfo info(m_executablePath);
        if (m_executablePath.isEmpty() || !info.exists() || !info.isFile() || !info.isExecutable() ||
            m_cachedSha512.isEmpty() || m_cachedLocaleName.isEmpty() || !m_catalog.isComplete())
            return false;
        return sha512ForFile(m_executablePath) == m_cachedSha512 &&
               m_cachedLocaleName == QLocale().name();
    }

    LibreSVIPValidationResult LibreSVIPManager::validateExecutable(const QString &path, QWindow *parent, bool showProgress) {
        std::unique_ptr<QObject> progress;
        if (showProgress) {
            progress.reset(createQuickObject("LibreSVIPProgressDialog", {
                {QStringLiteral("text"), tr("Validating LibreSVIP command-line tool...")},
                {QStringLiteral("cancellable"), true},
                {QStringLiteral("indeterminate"), true},
                {QStringLiteral("transientParent"), QVariant::fromValue(parent)},
            }));
            if (!progress) {
                LibreSVIPValidationResult result;
                result.errorMessage = tr("Failed to create the LibreSVIP progress dialog.");
                return result;
            }
            QMetaObject::invokeMethod(progress.get(), "show");
        }
        const auto result = validateExecutableInternal(path, [dialog = progress.get()] {
            return quickDialogWasCancelled(dialog);
        });
        if (progress)
            finishQuickDialog(progress.get(), result.cancelled ? QStringLiteral("cancelled")
                                                               : QStringLiteral("completed"));
        return result;
    }

    LibreSVIPValidationResult LibreSVIPManager::validateExecutableInternal(const QString &path,
                                                                            const CancellationCheck &isCancelled) {
        LibreSVIPValidationResult result;
        if (cancellationRequested(isCancelled)) {
            result.cancelled = true;
            return result;
        }
        const QFileInfo info(QDir::fromNativeSeparators(path));
        if (!info.exists()) {
            result.errorMessage = tr("The selected LibreSVIP executable does not exist.");
            return result;
        }
        if (!info.isFile()) {
            result.errorMessage = tr("The selected LibreSVIP path is not a file.");
            return result;
        }
        if (!info.isExecutable()) {
            result.errorMessage = tr("The selected LibreSVIP file is not executable.");
            return result;
        }
        const QString canonicalPath = info.canonicalFilePath();
        if (canonicalPath.isEmpty()) {
            result.errorMessage = tr("Failed to resolve the canonical path of the LibreSVIP executable.");
            return result;
        }
        const QFileInfo canonicalInfo(canonicalPath);
        bool digestCancelled = false;
        result.sha512 = sha512ForFile(canonicalPath, isCancelled, &digestCancelled);
        if (digestCancelled) {
            result.cancelled = true;
            return result;
        }
        if (result.sha512.isEmpty()) {
            result.errorMessage = tr("Failed to calculate the SHA512 digest of the LibreSVIP executable.");
            return result;
        }

        stopProcess(true);
        m_process = new QProcess(this);
        m_process->setProcessChannelMode(QProcess::MergedChannels);
        m_process->setWorkingDirectory(canonicalInfo.absolutePath());
        m_process->start(canonicalPath, {QStringLiteral("rpc"), QStringLiteral("server")});
        bool startCancelled = false;
        if (!waitForProcessStarted(m_process, 5000, isCancelled, &startCancelled)) {
            if (startCancelled) {
                result.cancelled = true;
                stopProcess(true);
                return result;
            }
            result.errorMessage = tr("Failed to start LibreSVIP: %1").arg(m_process->errorString());
            stopProcess(true);
            return result;
        }

        LibreSVIP::Conversion::Client client;
        if (!client.attachChannel(std::make_shared<QGrpcHttp2Channel>(kGrpcEndpoint))) {
            result.errorMessage = tr("Failed to initialize the LibreSVIP gRPC client.");
            stopProcess(true);
            return result;
        }

        QElapsedTimer startupTimer;
        startupTimer.start();
        QString rpcError;
        while (startupTimer.elapsed() < kCatalogTimeoutMs) {
            if (cancellationRequested(isCancelled)) {
                result.cancelled = true;
                stopProcess(true);
                return result;
            }
            if (m_process->state() == QProcess::NotRunning) {
                result.errorMessage = tr("LibreSVIP exited before its RPC server became ready: %1")
                                          .arg(QString::fromLocal8Bit(m_process->readAll()).trimmed());
                stopProcess(true);
                return result;
            }
            bool rpcCancelled = false;
            if (queryPluginInfos(client, LibreSVIP::PluginCategoryGadget::PluginCategory::INPUT,
                                 &result.catalog.inputs, &rpcError, 1000, isCancelled, &rpcCancelled))
                break;
            if (rpcCancelled) {
                result.cancelled = true;
                stopProcess(true);
                return result;
            }
            QCoreApplication::processEvents();
            QThread::msleep(100);
        }
        if (result.catalog.inputs.isEmpty()) {
            result.errorMessage = tr("LibreSVIP RPC server did not become ready: %1").arg(rpcError);
            stopProcess(true);
            return result;
        }
        if (m_process->state() == QProcess::NotRunning) {
            result.errorMessage = tr("LibreSVIP exited while starting its RPC server. Port 15150 may already be in use.");
            stopProcess(true);
            return result;
        }
        int remaining = kCatalogTimeoutMs - static_cast<int>(startupTimer.elapsed());
        if (remaining <= 0) {
            result.errorMessage = tr("Timed out while querying LibreSVIP plugins.");
            stopProcess(true);
            return result;
        }
        bool rpcCancelled = false;
        if (!queryPluginInfos(client, LibreSVIP::PluginCategoryGadget::PluginCategory::OUTPUT,
                              &result.catalog.outputs, &rpcError, remaining, isCancelled, &rpcCancelled)) {
            if (rpcCancelled) {
                result.cancelled = true;
                stopProcess(true);
                return result;
            }
            result.errorMessage = tr("Failed to query LibreSVIP plugins: %1").arg(rpcError);
            stopProcess(true);
            return result;
        }
        remaining = kCatalogTimeoutMs - static_cast<int>(startupTimer.elapsed());
        rpcCancelled = false;
        if (remaining <= 0 ||
            !queryPluginInfos(client, LibreSVIP::PluginCategoryGadget::PluginCategory::MIDDLEWARE,
                              &result.catalog.middlewares, &rpcError, remaining, isCancelled, &rpcCancelled)) {
            if (rpcCancelled) {
                result.cancelled = true;
                stopProcess(true);
                return result;
            }
            result.errorMessage = remaining <= 0
                                      ? tr("Timed out while querying LibreSVIP plugins.")
                                      : tr("Failed to query LibreSVIP plugins: %1").arg(rpcError);
            stopProcess(true);
            return result;
        }
        if (m_process->state() == QProcess::NotRunning) {
            result.errorMessage = tr("LibreSVIP exited while querying its plugins. Port 15150 may already be in use.");
            stopProcess(true);
            return result;
        }
        stopProcess();
        if (!result.catalog.find(LibreSVIPPluginCategory::Input, QStringLiteral("dspx")) ||
            !result.catalog.find(LibreSVIPPluginCategory::Output, QStringLiteral("dspx"))) {
            result.errorMessage = tr("This LibreSVIP installation does not provide both DSPX input and output plugins.");
            return result;
        }
        result.success = true;
        return result;
    }

    QString LibreSVIPManager::pickExecutable(QWindow *parent) const {
        QFileDialog dialog;
        dialog.setWindowTitle(tr("Select LibreSVIP command-line tool"));
        dialog.setAcceptMode(QFileDialog::AcceptOpen);
        dialog.setFileMode(QFileDialog::ExistingFile);
#ifdef Q_OS_WIN
        dialog.setNameFilters({tr("Executable files (*.exe)"), tr("All files (*)")});
#else
        dialog.setNameFilter(tr("All files (*)"));
#endif

        const QFileInfo configuredExecutable(m_executablePath);
        if (!m_executablePath.isEmpty()) {
            dialog.setDirectory(configuredExecutable.absolutePath());
            dialog.selectFile(configuredExecutable.fileName());
        }

        dialog.setWindowModality(Qt::WindowModal);
        dialog.winId();
        if (parent && dialog.windowHandle())
            dialog.windowHandle()->setTransientParent(parent);

        if (dialog.exec() != QDialog::Accepted)
            return {};
        return QDir::toNativeSeparators(dialog.selectedFiles().value(0));
    }

    bool LibreSVIPManager::browseAndConfigure(QWindow *parent, bool notifyRetry) {
        if (m_busy) {
            SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), parent, tr("LibreSVIP is busy"),
                                     tr("Another LibreSVIP operation is already running."));
            return false;
        }
        m_busy = true;
        const auto path = pickExecutable(parent);
        if (path.isEmpty()) {
            m_busy = false;
            return false;
        }
        const auto result = validateExecutable(path, parent, true);
        if (result.cancelled) {
            m_busy = false;
            return false;
        }
        if (!result.success) {
            showError(parent, tr("Invalid LibreSVIP command-line tool"), result.errorMessage);
            m_busy = false;
            return false;
        }
        if (!saveConfiguration(path, result)) {
            showError(parent, tr("Invalid LibreSVIP command-line tool"),
                      tr("Failed to resolve the canonical path of the LibreSVIP executable."));
            m_busy = false;
            return false;
        }
        m_busy = false;
        if (notifyRetry)
            showRetryMessage(parent);
        return true;
    }

    QString LibreSVIPManager::downloadedRoot() const {
        return Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::RuntimeData) + QStringLiteral("/libresvip");
    }

    QString LibreSVIPManager::downloadedExecutable() const {
#ifdef Q_OS_WINDOWS
        return downloadedRoot() + QStringLiteral("/libresvip-cli/libresvip-cli.exe");
#else
        return downloadedRoot() + QStringLiteral("/libresvip-cli/libresvip-cli");
#endif
    }

    QString LibreSVIPManager::downloadedVersion() const {
        QFile file(QDir(downloadedRoot()).filePath(QString::fromLatin1(kDownloadedVersionFileName)));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        const QString version = QString::fromUtf8(file.readAll()).trimmed();
        return SVS::Semver(version).isValid() ? version : QString{};
    }

    void LibreSVIPManager::checkForUpdates() {
        if (!m_autoCheckForUpdates || !downloadedInstallationExists() || m_updateCheckSession ||
            m_updateNotification)
            return;

        const QString installedVersionText = downloadedVersion();
        if (!SVS::Semver(installedVersionText).isValid()) {
            qCWarning(lcLibreSVIPManager) << "Skipping the LibreSVIP update check because the installed version is unavailable.";
            return;
        }

        m_updateCatalogData = new QBuffer(this);
        if (!m_updateCatalogData->open(QIODevice::WriteOnly)) {
            qCWarning(lcLibreSVIPManager) << "Failed to open the buffer for the LibreSVIP update catalog.";
            m_updateCatalogData->deleteLater();
            m_updateCatalogData = nullptr;
            return;
        }

        m_updateCheckSession = new SVS::DownloadSession(
            QUrl(QString::fromUtf8(DIFFSCOPE_LIBRESVIP_CATALOG_URL)), m_updateCatalogData, this);
        m_updateCheckSession->setUserAgent(QStringLiteral("DiffScope/%1").arg(QCoreApplication::applicationVersion()));
        auto *session = m_updateCheckSession;
        auto *catalogData = m_updateCatalogData;
        connect(session, &SVS::DownloadSession::networkErrorOccurred, this,
                [](QNetworkReply::NetworkError, const QString &description) {
                    qCWarning(lcLibreSVIPManager).noquote()
                        << "The background LibreSVIP update check encountered a network error:" << description;
                });
        connect(session, &SVS::DownloadSession::writeErrorOccurred, this,
                [](const QString &description) {
                    qCWarning(lcLibreSVIPManager).noquote()
                        << "The background LibreSVIP update check encountered a write error:" << description;
                });
        connect(session, &SVS::DownloadSession::finished, this, [this, session, catalogData] {
            if (session != m_updateCheckSession)
                return;

            const auto status = session->status();
            const QByteArray data = catalogData->data();
            m_updateCheckSession = nullptr;
            m_updateCatalogData = nullptr;
            session->deleteLater();
            catalogData->deleteLater();

            if (status == SVS::DownloadSession::Status::Cancelled) {
                qCDebug(lcLibreSVIPManager) << "The background LibreSVIP update check was cancelled.";
                return;
            }
            if (status != SVS::DownloadSession::Status::Completed)
                return;

            auto catalogResult = parseDownloadCatalog(data);
            if (!catalogResult.errorMessage.isEmpty()) {
                qCWarning(lcLibreSVIPManager).noquote()
                    << "The LibreSVIP update catalog could not be used:" << catalogResult.errorMessage;
                return;
            }
            if (!m_autoCheckForUpdates || !downloadedInstallationExists())
                return;

            const QString currentInstalledVersionText = downloadedVersion();
            const SVS::Semver currentInstalledVersion(currentInstalledVersionText);
            if (!currentInstalledVersion.isValid()) {
                qCWarning(lcLibreSVIPManager)
                    << "Skipping the LibreSVIP update notification because the installed version is unavailable.";
                return;
            }
            const auto &latest = catalogResult.artifacts.constFirst();
            if (latest.version > currentInstalledVersion)
                showUpdateNotification(latest.versionText, currentInstalledVersionText);
        });
        QTimer::singleShot(0, session, &SVS::DownloadSession::start);
    }

    bool LibreSVIPManager::downloadAndConfigure(QWindow *parent, bool notifyRetry) {
        cancelUpdateCheck();
        dismissUpdateNotification();
        if (m_busy) {
            SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), parent, tr("LibreSVIP is busy"),
                                     tr("Another LibreSVIP operation is already running."));
            return false;
        }
        m_busy = true;

        const bool hasDownloadedInstallation = downloadedInstallationExists();
        const QString installedVersion = downloadedVersion();
        std::unique_ptr<QObject> dialog(createQuickObject("LibreSVIPDownloadDialog", {
            {QStringLiteral("phase"), QStringLiteral("catalog")},
            {QStringLiteral("hasDownloadedInstallation"), hasDownloadedInstallation},
            {QStringLiteral("indeterminate"), true},
            {QStringLiteral("transientParent"), QVariant::fromValue(parent)},
        }));
        if (!dialog) {
            qCCritical(lcLibreSVIPManager) << "Failed to create the LibreSVIP download dialog.";
            showError(parent, tr("Download failed"), tr("Failed to create the LibreSVIP download dialog."));
            m_busy = false;
            return false;
        }

        const auto setDialogProgress = [dialog = dialog.get()](qint64 received, qint64 total) {
            dialog->setProperty("bytesReceived", received);
            dialog->setProperty("bytesTotal", total);
            dialog->setProperty("indeterminate", total <= 0);
            dialog->setProperty("progress", total > 0
                                                ? static_cast<double>(received) / static_cast<double>(total)
                                                : 0.0);
        };
        QString pendingErrorTitle;
        QString pendingErrorMessage;
        const auto queueDialogError = [dialog = dialog.get(), &pendingErrorTitle, &pendingErrorMessage]
                                      (const QString &title, const QString &message) {
            pendingErrorTitle = title;
            pendingErrorMessage = message;
            finishQuickDialog(dialog, QStringLiteral("error"));
        };
        const auto showPendingError = [this, parent, &pendingErrorTitle, &pendingErrorMessage] {
            const QString title = pendingErrorTitle.isEmpty() ? tr("LibreSVIP error") : pendingErrorTitle;
            const QString message = pendingErrorMessage.isEmpty() ? tr("The LibreSVIP operation failed.")
                                                                   : pendingErrorMessage;
            qCWarning(lcLibreSVIPManager).noquote() << title << ':' << message;
            showError(parent, title, message);
        };
        const auto reportError = [this, parent, dialogObject = dialog.get()]
                                 (const QString &title, const QString &message) {
            qCWarning(lcLibreSVIPManager).noquote() << title << ':' << message;
            finishQuickDialog(dialogObject, QStringLiteral("error"));
            showError(parent, title, message);
        };

        QList<LibreSVIPDownloadArtifact> artifacts;
        QBuffer catalogData;
        catalogData.open(QIODevice::WriteOnly);
        SVS::DownloadSession catalogSession(QUrl(QString::fromUtf8(DIFFSCOPE_LIBRESVIP_CATALOG_URL)), &catalogData);
        catalogSession.setUserAgent(QStringLiteral("DiffScope/%1").arg(QCoreApplication::applicationVersion()));
        QString catalogDownloadError;
        connect(&catalogSession, &SVS::DownloadSession::progressChanged, dialog.get(), setDialogProgress);
        connect(&catalogSession, &SVS::DownloadSession::networkErrorOccurred, dialog.get(),
                [this, &catalogDownloadError, queueDialogError]
                (QNetworkReply::NetworkError, const QString &description) {
                    catalogDownloadError = description;
                    queueDialogError(tr("Download failed"), description);
                });
        connect(&catalogSession, &SVS::DownloadSession::writeErrorOccurred, dialog.get(),
                [this, &catalogDownloadError, queueDialogError](const QString &description) {
                    catalogDownloadError = description;
                    queueDialogError(tr("Download failed"), description);
                });
        connect(&catalogSession, &SVS::DownloadSession::finished, dialog.get(),
                [this, &catalogSession, &catalogData, &catalogDownloadError, &artifacts, installedVersion,
                 queueDialogError, dialogObject = dialog.get()] {
                    if (catalogSession.status() == SVS::DownloadSession::Status::Cancelled) {
                        finishQuickDialog(dialogObject, QStringLiteral("cancelled"));
                        return;
                    }
                    if (catalogSession.status() != SVS::DownloadSession::Status::Completed) {
                        queueDialogError(tr("Download failed"),
                                         catalogDownloadError.isEmpty()
                                             ? tr("The LibreSVIP download catalog could not be retrieved.")
                                             : catalogDownloadError);
                        return;
                    }

                    auto catalogResult = parseDownloadCatalog(catalogData.data());
                    if (!catalogResult.errorMessage.isEmpty()) {
                        queueDialogError(tr("Download failed"), catalogResult.errorMessage);
                        return;
                    }
                    artifacts = std::move(catalogResult.artifacts);

                    QStringList versionItems;
                    versionItems.reserve(artifacts.size());
                    for (const auto &artifact : std::as_const(artifacts)) {
                        const QString display = artifact.versionText == installedVersion
                                                    ? tr("%1 (Downloaded)").arg(artifact.versionText)
                                                    : artifact.versionText;
                        versionItems.append(display);
                    }
                    dialogObject->setProperty("versionItems", versionItems);
                    dialogObject->setProperty("selectedIndex", 0);
                    dialogObject->setProperty("phase", QStringLiteral("selection"));
                });
        QTimer::singleShot(0, &catalogSession, &SVS::DownloadSession::start);

        const QVariant selectionResult = SVS::MessageBox::customExec(dialog.get());
        if (selectionResult.toString() == QStringLiteral("error")) {
            showPendingError();
            m_busy = false;
            return false;
        }
        if (selectionResult.toString() == QStringLiteral("cancelled")) {
            catalogSession.cancel();
            m_busy = false;
            return false;
        }
        if (selectionResult.toString() != QStringLiteral("download")) {
            m_busy = false;
            return false;
        }
        const int selectedIndex = dialog->property("selectedIndex").toInt();
        if (selectedIndex < 0 || selectedIndex >= artifacts.size()) {
            reportError(tr("Download failed"), tr("The selected LibreSVIP version is not available."));
            m_busy = false;
            return false;
        }
        const LibreSVIPDownloadArtifact selectedArtifact = artifacts.at(selectedIndex);

        const QString tempRoot = Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::TempData);
        QDir().mkpath(tempRoot);
        QTemporaryFile archiveFile(tempRoot + QStringLiteral("/libresvip-download-XXXXXX"));
        if (!archiveFile.open()) {
            reportError(tr("Download failed"), archiveFile.errorString());
            m_busy = false;
            return false;
        }

        dialog->setProperty("phase", QStringLiteral("download"));
        setDialogProgress(0, -1);
        SVS::DownloadSession session(selectedArtifact.url, &archiveFile);
        session.setUserAgent(QStringLiteral("DiffScope/%1").arg(QCoreApplication::applicationVersion()));
        QString downloadError;
        connect(&session, &SVS::DownloadSession::progressChanged, dialog.get(), setDialogProgress);
        connect(&session, &SVS::DownloadSession::networkErrorOccurred, dialog.get(),
                [this, &downloadError, queueDialogError]
                (QNetworkReply::NetworkError, const QString &description) {
                    downloadError = description;
                    queueDialogError(tr("Download failed"), description);
                });
        connect(&session, &SVS::DownloadSession::writeErrorOccurred, dialog.get(),
                [this, &downloadError, queueDialogError](const QString &description) {
                    downloadError = description;
                    queueDialogError(tr("Download failed"), description);
                });
        connect(&session, &SVS::DownloadSession::finished, dialog.get(),
                [this, &session, &downloadError, queueDialogError, dialogObject = dialog.get()] {
                    if (session.status() == SVS::DownloadSession::Status::Completed) {
                        yieldQuickDialog(dialogObject, QStringLiteral("downloaded"));
                    } else if (session.status() == SVS::DownloadSession::Status::Cancelled) {
                        finishQuickDialog(dialogObject, QStringLiteral("cancelled"));
                    } else {
                        queueDialogError(tr("Download failed"),
                                         downloadError.isEmpty()
                                             ? tr("LibreSVIP could not be downloaded.")
                                             : downloadError);
                    }
                });
        QTimer::singleShot(0, &session, &SVS::DownloadSession::start);
        const QString downloadResult = SVS::MessageBox::customExec(dialog.get()).toString();
        if (downloadResult == QStringLiteral("error")) {
            showPendingError();
            m_busy = false;
            return false;
        }
        if (downloadResult == QStringLiteral("cancelled")) {
            session.cancel();
            m_busy = false;
            return false;
        }
        if (downloadResult != QStringLiteral("downloaded") ||
            session.status() != SVS::DownloadSession::Status::Completed) {
            m_busy = false;
            return false;
        }
        if (!archiveFile.flush()) {
            reportError(tr("Download failed"), archiveFile.errorString());
            m_busy = false;
            return false;
        }
        archiveFile.close();

        const QString dataRoot = Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::RuntimeData);
        QDir().mkpath(dataRoot);
        const QString stageRoot = dataRoot + QStringLiteral("/libresvip.stage-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString extractionError;
        dialog->setProperty("phase", QStringLiteral("install"));
        dialog->setProperty("statusText", tr("Installing LibreSVIP..."));
        dialog->setProperty("indeterminate", true);
        const CancellationCheck installCancelled = [dialog = dialog.get()] {
            return quickDialogWasCancelled(dialog);
        };
        bool extractionCancelled = false;
        if (!extractArchive(archiveFile.fileName(), stageRoot, &extractionError,
                            installCancelled, &extractionCancelled)) {
            QDir(stageRoot).removeRecursively();
            if (!extractionCancelled)
                reportError(tr("Installation failed"), extractionError);
            m_busy = false;
            return false;
        }
#ifdef Q_OS_WINDOWS
        const QString stagedExecutable = stageRoot + QStringLiteral("/libresvip-cli/libresvip-cli.exe");
#else
        const QString stagedExecutable = stageRoot + QStringLiteral("/libresvip-cli/libresvip-cli");
#endif
        dialog->setProperty("statusText", tr("Validating LibreSVIP command-line tool..."));
        auto validation = validateExecutableInternal(stagedExecutable, installCancelled);
        if (validation.cancelled) {
            QDir(stageRoot).removeRecursively();
            m_busy = false;
            return false;
        }
        if (!validation.success) {
            QDir(stageRoot).removeRecursively();
            reportError(tr("Installation failed"), validation.errorMessage);
            m_busy = false;
            return false;
        }

        QSaveFile versionFile(QDir(stageRoot).filePath(QString::fromLatin1(kDownloadedVersionFileName)));
        const QByteArray versionData = selectedArtifact.versionText.toUtf8() + '\n';
        if (!versionFile.open(QIODevice::WriteOnly) || versionFile.write(versionData) != versionData.size() ||
            !versionFile.commit()) {
            const QString error = versionFile.errorString();
            QDir(stageRoot).removeRecursively();
            reportError(tr("Installation failed"),
                        tr("Failed to record the downloaded LibreSVIP version: %1").arg(error));
            m_busy = false;
            return false;
        }
        QCoreApplication::processEvents();
        if (cancellationRequested(installCancelled)) {
            QDir(stageRoot).removeRecursively();
            m_busy = false;
            return false;
        }

        const QString finalRoot = downloadedRoot();
        const QString backupRoot = dataRoot + QStringLiteral("/libresvip.backup-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        bool hadExisting = QDir(finalRoot).exists();
        if (hadExisting && !QDir().rename(finalRoot, backupRoot)) {
            QDir(stageRoot).removeRecursively();
            reportError(tr("Installation failed"), tr("Failed to replace the existing LibreSVIP installation."));
            m_busy = false;
            return false;
        }
        if (!QDir().rename(stageRoot, finalRoot)) {
            if (hadExisting)
                QDir().rename(backupRoot, finalRoot);
            QDir(stageRoot).removeRecursively();
            reportError(tr("Installation failed"),
                        tr("Failed to move LibreSVIP into the application data directory."));
            m_busy = false;
            return false;
        }
        if (!saveConfiguration(downloadedExecutable(), validation)) {
            const bool movedFailedInstallation = QDir().rename(finalRoot, stageRoot);
            const bool restoredExisting = !hadExisting || QDir().rename(backupRoot, finalRoot);
            if (movedFailedInstallation)
                QDir(stageRoot).removeRecursively();
            reportError(tr("Installation failed"),
                        restoredExisting
                            ? tr("Failed to resolve the canonical path of the LibreSVIP executable.")
                            : tr("Failed to restore the previous LibreSVIP installation."));
            m_busy = false;
            return false;
        }
        if (hadExisting)
            QDir(backupRoot).removeRecursively();
        finishQuickDialog(dialog.get(), QStringLiteral("completed"));
        m_busy = false;
        if (notifyRetry)
            showRetryMessage(parent);
        return true;
    }

    bool LibreSVIPManager::clearExecutablePath(QWindow *parent) {
        if (m_busy) {
            SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), parent, tr("LibreSVIP is busy"),
                                     tr("Another LibreSVIP operation is already running."));
            return false;
        }
        if (m_executablePath.isEmpty())
            return true;
        clearConfiguration();
        return true;
    }

    bool LibreSVIPManager::removeDownloadedInstallation(QWindow *parent) {
        if (m_busy) {
            SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), parent, tr("LibreSVIP is busy"),
                                     tr("Another LibreSVIP operation is already running."));
            return false;
        }
        if (!QDir(downloadedRoot()).exists())
            return true;
        if (SVS::MessageBox::question(Core::RuntimeInterface::qmlEngine(), parent,
                                      tr("Delete downloaded LibreSVIP"),
                                      tr("Delete the downloaded LibreSVIP command-line tool and all files in its installation directory?"),
                                      SVS::SVSCraft::Yes | SVS::SVSCraft::No, SVS::SVSCraft::No) != SVS::SVSCraft::Yes)
            return false;
        m_busy = true;
        const bool clearCurrent = pathIsWithin(downloadedRoot(), m_executablePath);
        if (!QDir(downloadedRoot()).removeRecursively()) {
            showError(parent, tr("Deletion failed"), tr("Failed to delete the downloaded LibreSVIP installation."));
            m_busy = false;
            return false;
        }
        if (clearCurrent)
            clearConfiguration();
        else
            Q_EMIT configurationChanged();
        m_busy = false;
        return true;
    }

    bool LibreSVIPManager::runPreExecCheck() {
        QWindow *parent = defaultParentWindow();
        if (m_executablePath.isEmpty()) {
            QVariantList buttons{
                QVariantMap{{QStringLiteral("id"), QStringLiteral("download")}, {QStringLiteral("text"), tr("Download LibreSVIP")}},
                QVariantMap{{QStringLiteral("id"), QStringLiteral("browse")}, {QStringLiteral("text"), tr("Browse...")}},
                QVariant::fromValue(SVS::SVSCraft::Cancel),
            };
            std::unique_ptr<QObject> dialog(createQuickObject("LibreSVIPMessageBoxDialog", {
                {QStringLiteral("text"), tr("LibreSVIP is not configured")},
                {QStringLiteral("informativeText"), tr("Configure the LibreSVIP command-line tool before importing or exporting through LibreSVIP.")},
                {QStringLiteral("buttons"), buttons},
                {QStringLiteral("primaryButton"), QStringLiteral("download")},
                {QStringLiteral("transientParent"), QVariant::fromValue(parent)},
            }));
            const QString choice = dialog ? SVS::MessageBox::customExec(dialog.get()).toString() : QString();
            QPointer<QWindow> guardedParent(parent);
            if (choice == QStringLiteral("download")) {
                QTimer::singleShot(0, this, [this, guardedParent] { downloadAndConfigure(guardedParent ? guardedParent.data() : defaultParentWindow(), true); });
            } else if (choice == QStringLiteral("browse")) {
                QTimer::singleShot(0, this, [this, guardedParent] { browseAndConfigure(guardedParent ? guardedParent.data() : defaultParentWindow(), true); });
            }
            return false;
        }
        if (cacheIsCurrent())
            return true;

        if (m_busy)
            return false;
        m_busy = true;
        const auto validation = validateExecutable(m_executablePath, parent, true);
        m_busy = false;
        if (validation.cancelled)
            return false;
        if (validation.success) {
            if (!saveConfiguration(m_executablePath, validation)) {
                showError(parent, tr("LibreSVIP command-line tool is unavailable"),
                          tr("Failed to resolve the canonical path of the LibreSVIP executable."));
                return false;
            }
            showRetryMessage(parent);
            return false;
        }
        qCWarning(lcLibreSVIPManager) << "LibreSVIP validation failed:" << validation.errorMessage;
        const auto button = SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), parent,
                                                       tr("LibreSVIP command-line tool is unavailable"),
                                                       validation.errorMessage,
                                                       SVS::SVSCraft::Open | SVS::SVSCraft::Cancel,
                                                       SVS::SVSCraft::Open);
        if (button == SVS::SVSCraft::Open) {
            QPointer<QWindow> guardedParent(parent);
            QTimer::singleShot(0, this, [guardedParent] {
                Core::CoreInterface::execSettingsDialog(QString::fromLatin1(settingsPageId()), guardedParent.data());
            });
        }
        return false;
    }

    LibreSVIPConversionResult LibreSVIPManager::convert(const LibreSVIPConversionRequestData &requestData, QWindow *parent) {
        LibreSVIPConversionResult result;
        if (m_busy) {
            result.errorMessage = tr("Another LibreSVIP operation is already running.");
            return result;
        }
        if (requestData.inputData.size() > kMaximumInputSize) {
            result.errorMessage = tr("The input file is larger than the 512 MiB LibreSVIP conversion limit.");
            return result;
        }
        if (!cacheIsCurrent()) {
            result.errorMessage = tr("The configured LibreSVIP command-line tool is no longer valid.");
            return result;
        }
        m_busy = true;
        std::unique_ptr<QObject> progress(createQuickObject("LibreSVIPProgressDialog", {
            {QStringLiteral("text"), tr("Converting project with LibreSVIP...")},
            {QStringLiteral("cancellable"), true},
            {QStringLiteral("indeterminate"), true},
            {QStringLiteral("transientParent"), QVariant::fromValue(parent)},
        }));
        if (!progress) {
            result.errorMessage = tr("Failed to create the LibreSVIP progress dialog.");
            m_busy = false;
            return result;
        }
        QMetaObject::invokeMethod(progress.get(), "show");
        QCoreApplication::processEvents();

        stopProcess(true);
        m_process = new QProcess(this);
        m_process->setProcessChannelMode(QProcess::MergedChannels);
        m_process->setWorkingDirectory(QFileInfo(m_executablePath).absolutePath());
        m_process->start(m_executablePath, {QStringLiteral("rpc"), QStringLiteral("server")});
        const CancellationCheck conversionCancelled = [dialog = progress.get()] {
            return quickDialogWasCancelled(dialog);
        };
        bool startCancelled = false;
        if (!waitForProcessStarted(m_process, 5000, conversionCancelled, &startCancelled)) {
            if (startCancelled) {
                stopProcess(true);
                result.cancelled = true;
                m_busy = false;
                return result;
            }
            result.errorMessage = tr("Failed to start LibreSVIP: %1").arg(m_process->errorString());
            stopProcess(true);
            m_busy = false;
            return result;
        }

        LibreSVIP::Conversion::Client client;
        if (!client.attachChannel(std::make_shared<QGrpcHttp2Channel>(kGrpcEndpoint))) {
            result.errorMessage = tr("Failed to initialize the LibreSVIP gRPC client.");
            stopProcess(true);
            m_busy = false;
            return result;
        }
        QElapsedTimer startupTimer;
        startupTimer.start();
        QList<LibreSVIPPluginInfo> ignored;
        QString rpcError;
        while (startupTimer.elapsed() < kCatalogTimeoutMs && m_process->state() != QProcess::NotRunning) {
            bool rpcCancelled = false;
            if (queryPluginInfos(client, LibreSVIP::PluginCategoryGadget::PluginCategory::INPUT,
                                 &ignored, &rpcError, 1000, conversionCancelled, &rpcCancelled))
                break;
            if (rpcCancelled) {
                stopProcess(true);
                result.cancelled = true;
                m_busy = false;
                return result;
            }
            QCoreApplication::processEvents();
            if (quickDialogWasCancelled(progress.get())) {
                stopProcess(true);
                result.cancelled = true;
                m_busy = false;
                return result;
            }
            QThread::msleep(100);
        }
        if (quickDialogWasCancelled(progress.get())) {
            stopProcess(true);
            result.cancelled = true;
            m_busy = false;
            return result;
        }
        if (ignored.isEmpty()) {
            result.errorMessage = tr("LibreSVIP RPC server did not become ready: %1").arg(rpcError);
            stopProcess(true);
            m_busy = false;
            return result;
        }
        if (m_process->state() == QProcess::NotRunning) {
            result.errorMessage = tr("LibreSVIP exited while starting its RPC server. Port 15150 may already be in use.");
            stopProcess(true);
            m_busy = false;
            return result;
        }

        LibreSVIP::ConversionGroup group;
        const QString groupId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        group.setGroupId(groupId);
        group.setFileContents(QByteArrayList{requestData.inputData});
        LibreSVIP::ConversionRequest request;
        request.setInputFormat(requestData.inputIdentifier);
        request.setOutputFormat(requestData.outputIdentifier);
        request.setMode(LibreSVIP::ConversionModeGadget::ConversionMode::DIRECT);
        request.setMaxTrackCount(1);
        request.setGroups(LibreSVIP::ConversionGroupRepeated{group});
        request.setInputOptions(compactJson(requestData.inputOptions));
        request.setOutputOptions(compactJson(requestData.outputOptions));
        LibreSVIP::ConversionRequest::MiddlewareOptionsEntry middlewareOptions;
        for (auto it = requestData.middlewareOptions.cbegin(); it != requestData.middlewareOptions.cend(); ++it)
            middlewareOptions.insert(it.key(), compactJson(it.value()));
        request.setMiddlewareOptions(std::move(middlewareOptions));

        QGrpcCallOptions options;
        options.setDeadlineTimeout(std::chrono::milliseconds(kConversionTimeoutMs));
        auto reply = client.Convert(request, options);
        std::optional<LibreSVIP::ConversionResponse> response;
        QGrpcStatus finalStatus;
        connect(reply.get(), &QGrpcCallReply::finished, progress.get(), [&](const QGrpcStatus &status) {
            finalStatus = status;
            if (status.isOk())
                response = reply->read<LibreSVIP::ConversionResponse>();
            finishQuickDialog(progress.get(), QStringLiteral("completed"));
        });
        const QString dialogResult = SVS::MessageBox::customExec(progress.get()).toString();
        if (dialogResult == QStringLiteral("cancelled")) {
            reply->cancel();
            stopProcess(true);
            result.cancelled = true;
            m_busy = false;
            return result;
        }
        stopProcess();
        m_busy = false;
        if (!finalStatus.isOk()) {
            qCWarning(lcLibreSVIPManager).noquote() << "LibreSVIP gRPC conversion error:" << finalStatus.message();
            result.errorMessage = userFacingRpcError(finalStatus.message());
            if (result.errorMessage.isEmpty())
                result.errorMessage = tr("LibreSVIP conversion failed.");
            return result;
        }
        if (!response) {
            result.errorMessage = tr("Failed to decode the LibreSVIP conversion response.");
            return result;
        }
        const auto groupIt = response->groupResults().constFind(groupId);
        if (groupIt == response->groupResults().cend()) {
            result.errorMessage = tr("LibreSVIP did not return a result for this conversion.");
            return result;
        }
        const auto &groupResult = groupIt.value();
        result.warningMessages = groupResult.warningMessages();
        if (!groupResult.success()) {
            qCWarning(lcLibreSVIPManager).noquote() << "LibreSVIP conversion traceback:" << groupResult.errorMessage();
            result.errorMessage = userFacingRpcError(groupResult.errorMessage());
            if (result.errorMessage.isEmpty())
                result.errorMessage = tr("LibreSVIP conversion failed.");
            return result;
        }
        result.outputData = groupResult.fileContents();
        if (result.outputData.isEmpty()) {
            result.errorMessage = tr("LibreSVIP returned no output data.");
            return result;
        }
        result.success = true;
        return result;
    }

    void LibreSVIPManager::stopProcess(bool force) {
        if (!m_process)
            return;
        if (m_process->state() != QProcess::NotRunning) {
            if (force) {
                m_process->kill();
            } else {
                m_process->terminate();
                if (!m_process->waitForFinished(1500))
                    m_process->kill();
            }
            m_process->waitForFinished(1000);
        }
        delete m_process;
        m_process = nullptr;
    }

    QWindow *LibreSVIPManager::defaultParentWindow() const {
        if (auto window = QGuiApplication::focusWindow())
            return window;
        if (auto windowInterface = Core::CoreInterface::windowSystem()->firstWindow())
            return windowInterface->window();
        return nullptr;
    }

    void LibreSVIPManager::cancelUpdateCheck() {
        auto *session = std::exchange(m_updateCheckSession, nullptr);
        auto *catalogData = std::exchange(m_updateCatalogData, nullptr);
        if (session) {
            disconnect(session, nullptr, this, nullptr);
            session->cancel();
            session->deleteLater();
        }
        if (catalogData)
            catalogData->deleteLater();
    }

    void LibreSVIPManager::dismissUpdateNotification() {
        auto *message = std::exchange(m_updateNotification, nullptr);
        if (message)
            message->close();
    }

    void LibreSVIPManager::showUpdateNotification(const QString &latestVersion, const QString &installedVersion) {
        Q_UNUSED(installedVersion);
        dismissUpdateNotification();
        auto *message = new Core::NotificationMessage(this);
        m_updateNotification = message;
        message->setIcon(SVS::SVSCraft::Information);
        message->setTitle(tr("LibreSVIP update available"));
        message->setText(tr("LibreSVIP %1 is available.").arg(latestVersion));
        message->setButtons({tr("Update")});
        message->setPrimaryButton(0);
        message->setClosable(true);
        message->setAllowDoNotShowAgain(true);
        connect(message, &Core::NotificationMessage::buttonClicked, this, [this, message](int index) {
            if (index != 0)
                return;
            if (m_updateNotification == message)
                dismissUpdateNotification();
            else
                message->close();
            QTimer::singleShot(0, this, [this] { downloadAndConfigure(defaultParentWindow()); });
        });
        connect(message, &Core::NotificationMessage::doNotShowAgainRequested, this, [this, message] {
            setAutoCheckForUpdates(false);
            if (m_updateNotification == message)
                dismissUpdateNotification();
        });
        connect(message, &Core::NotificationMessage::closed, this, [this, message] {
            if (m_updateNotification == message)
                m_updateNotification = nullptr;
            message->deleteLater();
        });
        Core::CoreInterface::sendNotification(message, Core::CoreInterface::AutoHide);
    }

    void LibreSVIPManager::showRetryMessage(QWindow *parent) const {
        SVS::MessageBox::information(Core::RuntimeInterface::qmlEngine(), parent,
                                     tr("LibreSVIP configured"),
                                     tr("LibreSVIP is ready. Please retry the import or export operation."));
    }

    void LibreSVIPManager::showError(QWindow *parent, const QString &title, const QString &message) const {
        SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), parent, title, message);
    }

}
