// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#include "LibreSVIPManager.h"

#include <chrono>
#include <memory>
#include <optional>
#include <utility>

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
#include <QLoggingCategory>
#include <QLocale>
#include <QPointer>
#include <QProcess>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QScopedValueRollback>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVariant>

#include <QtGrpc/QGrpcCallOptions>
#include <QtGrpc/QGrpcCallReply>
#include <QtGrpc/QGrpcHttp2Channel>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/windowsystem.h>

#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftCore/Semver.h>

#ifdef MessageBox
#  undef MessageBox
#endif

#include <SVSCraftQuick/MessageBox.h>

#include <appinstall/InstallationManager.h>
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
    static const QUrl kGrpcEndpoint(QStringLiteral("http://127.0.0.1:15150"));
    using CancellationCheck = std::function<bool()>;

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

    static QString userFacingRpcError(const QString &error) {
        const auto lines = error.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (auto it = lines.crbegin(); it != lines.crend(); ++it) {
            const auto line = it->trimmed();
            if (!line.isEmpty() && !line.startsWith(QStringLiteral("Traceback")))
                return line;
        }
        return error.trimmed();
    }

    static QObject *createQuickObject(const char *componentName, const QVariantMap &properties) {
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.LibreSVIPFormatConverter", componentName);
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties(properties);
        if (!object) {
            qFatal() << component.errorString();
        }
        return object;
    }

    static void finishQuickDialog(QObject *dialog, const QVariant &result) {
        if (dialog)
            QMetaObject::invokeMethod(dialog, "finish", Q_ARG(QVariant, result));
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

    LibreSVIPManager::LibreSVIPManager(QObject *parent) : QObject(parent) {
        Q_ASSERT(!s_instance);
        s_instance = this;
        AppInstall::ProductConfiguration product;
        product.id = QStringLiteral("libresvip-cli");
        product.displayName = tr("LibreSVIP");
#ifdef Q_OS_WIN
        product.executableRelativePath = QStringLiteral("libresvip-cli.exe");
#else
        product.executableRelativePath = QStringLiteral("libresvip-cli");
#endif
        product.catalogUrl = QUrl(QString::fromUtf8(DIFFSCOPE_LIBRESVIP_CATALOG_BASE_URL));
        product.validateInstallation = [this](const AppInstall::InstallationCandidate &candidate, const AppInstall::InstallationOperation &operation) {
            operation.setStatusText(tr("Validating LibreSVIP command-line tool..."));
            const auto result = validateExecutableInternal(candidate.executablePath, operation.isCancelled);
            return AppInstall::InstallationValidationResult{result.success, result.cancelled, result.errorMessage, QVariant::fromValue(result)};
        };
        m_installations = new AppInstall::InstallationManager(product, this);
        connect(m_installations, &AppInstall::InstallationManager::installationsChanged, this, &LibreSVIPManager::installationsChanged);
        connect(m_installations, &AppInstall::InstallationManager::latestVersionsReady, this, [this](const QVariantMap &versions, const QString &error) {
            if (!std::exchange(m_updateCheckPending, false))
                return;
            if (!error.isEmpty()) {
                qCWarning(lcLibreSVIPManager) << "Could not retrieve the LibreSVIP release catalog:" << error;
                return;
            }
            if (!m_autoCheckForUpdates || !selectedInstallationIsDownloaded())
                return;
            const auto current = selectedInstallationVersion();
            const auto latest = versions.value(m_installations->channel(m_installations->selection())).toString();
            if (SVS::Semver(current).isValid() && SVS::Semver(latest).isValid() && SVS::Semver(latest) > SVS::Semver(current))
                showUpdateNotification(latest, current);
        });
        loadConfiguration();
    }

    LibreSVIPManager::~LibreSVIPManager() {
        cancelUpdateCheck();
        dismissUpdateNotification();
        stopProcess(true);
        m_installations->shutdown();
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

    bool LibreSVIPManager::selectedInstallationIsDownloaded() const {
        return m_installations->selection().id.startsWith(QStringLiteral("downloaded:")) && QFileInfo(m_installations->executablePath()).isFile();
    }

    QVariantList LibreSVIPManager::installations() const {
        return m_installations->installations(m_installations->selection());
    }

    QString LibreSVIPManager::installationId() const {
        return m_installations->selection().id;
    }

    bool LibreSVIPManager::selectInstallation(const QString &id, QWindow *parent) {
        if (m_busy || id == installationId())
            return false;
        cancelUpdateCheck();
        dismissUpdateNotification();
        QScopedValueRollback guard(m_busy, true);
        auto selection = m_installations->selection();
        selection.id = id;
        if (id.isEmpty()) {
            m_installations->setSelection(selection);
            clearConfiguration();
            return true;
        }
        const auto path = m_installations->executablePath(selection);
        const auto validation = validateExecutable(path, parent, true);
        if (validation.cancelled)
            return false;
        if (!validation.success) {
            showError(parent, tr("Invalid LibreSVIP Command-Line Tool"), validation.errorMessage);
            return false;
        }
        if (!saveConfiguration(selection, validation)) {
            showError(parent, tr("Invalid LibreSVIP Command-Line Tool"), tr("Failed to resolve the canonical path of the LibreSVIP executable."));
            return false;
        }
        return true;
    }

    bool LibreSVIPManager::removeLocalInstallation() {
        if (m_busy)
            return false;
        auto selection = m_installations->selection();
        if (!m_installations->removeLocalInstallation(selection))
            return false;
        cancelUpdateCheck();
        dismissUpdateNotification();
        m_installations->setSelection(selection);
        clearConfiguration();
        return true;
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
        m_executablePath = m_installations->executablePath();
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QString::fromLatin1(kSettingsGroup));
        m_autoCheckForUpdates = settings->value(QStringLiteral("autoCheckForUpdates"), true).toBool();
        const auto configuration = QJsonDocument::fromJson(settings->value(QStringLiteral("configuration")).toByteArray()).object();
        if (configuration.value(QStringLiteral("cacheVersion")).toInt() == kCacheVersion &&
            configuration.value(QStringLiteral("installationId")).toString() == installationId()) {
            m_cachedSha512 = QByteArray::fromHex(configuration.value(QStringLiteral("sha512")).toString().toLatin1());
            m_cachedLocaleName = configuration.value(QStringLiteral("localeName")).toString();
            m_catalog = LibreSVIPPluginCatalog::fromJson(configuration.value(QStringLiteral("pluginCatalog")).toObject());
        }
        settings->endGroup();
    }

    bool LibreSVIPManager::saveConfiguration(const AppInstall::InstallationSelection &selection, const LibreSVIPValidationResult &result) {
        const auto path = m_installations->executablePath(selection);
        const QString canonicalPath = canonicalNativePath(path);
        if (canonicalPath.isEmpty()) {
            qCCritical(lcLibreSVIPManager) << "Failed to resolve the canonical LibreSVIP executable path:" << path;
            return false;
        }
        m_executablePath = canonicalPath;
        m_cachedSha512 = result.sha512;
        m_cachedLocaleName = QLocale().name();
        m_catalog = result.catalog;
        m_installations->setSelection(selection);
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QString::fromLatin1(kSettingsGroup));
        const QJsonObject configuration{
            {QStringLiteral("cacheVersion"), kCacheVersion},
            {QStringLiteral("installationId"), installationId()},
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
                {QStringLiteral("transientParent"), QVariant::fromValue(parent)},
            }));
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
        dialog.setWindowTitle(tr("Select LibreSVIP Command-Line Tool"));
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
        if (m_busy)
            return false;
        cancelUpdateCheck();
        dismissUpdateNotification();
        QScopedValueRollback guard(m_busy, true);
        const auto path = pickExecutable(parent);
        if (path.isEmpty())
            return false;
        const auto validation = validateExecutable(path, parent, true);
        if (validation.cancelled)
            return false;
        if (!validation.success) {
            showError(parent, tr("Invalid LibreSVIP Command-Line Tool"), validation.errorMessage);
            return false;
        }
        auto selection = m_installations->selection();
        QString error;
        if (!m_installations->addLocalInstallation(selection, path, &error)) {
            showError(parent, tr("Invalid LibreSVIP Command-Line Tool"), error);
            return false;
        }
        if (!saveConfiguration(selection, validation)) {
            showError(parent, tr("Invalid LibreSVIP Command-Line Tool"), tr("Failed to resolve the canonical path of the LibreSVIP executable."));
            return false;
        }
        if (notifyRetry)
            showRetryMessage(parent);
        return true;
    }

    QString LibreSVIPManager::selectedInstallationVersion() const {
        return m_installations->version(m_installations->selection());
    }

    void LibreSVIPManager::checkForUpdates() {
        if (!m_autoCheckForUpdates || !selectedInstallationIsDownloaded() || m_updateCheckPending || m_updateNotification || m_busy)
            return;
        m_updateCheckPending = true;
        m_installations->fetchLatestVersions();
    }

    bool LibreSVIPManager::downloadAndConfigure(QWindow *parent, bool notifyRetry) {
        if (m_busy)
            return false;
        cancelUpdateCheck();
        dismissUpdateNotification();
        QScopedValueRollback guard(m_busy, true);
        const auto result = m_installations->download(parent);
        if (result.id.isEmpty())
            return false;
        auto selection = m_installations->selection();
        selection.id = result.id;
        const auto validation = result.validationData.value<LibreSVIPValidationResult>();
        if (!saveConfiguration(selection, validation)) {
            showError(parent, tr("Installation Failed"), tr("Failed to resolve the canonical path of the LibreSVIP executable."));
            return false;
        }
        if (notifyRetry)
            showRetryMessage(parent);
        return true;
    }

    bool LibreSVIPManager::uninstallSelectedInstallation(QWindow *parent) {
        if (m_busy || !selectedInstallationIsDownloaded())
            return false;
        cancelUpdateCheck();
        dismissUpdateNotification();
        QScopedValueRollback guard(m_busy, true);
        const bool removed = m_installations->uninstall(installationId(), parent, [this](const QString &, QString *) {
            stopProcess(true);
            return true;
        });
        if (removed)
            clearConfiguration();
        return removed;
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
            const QString choice = SVS::MessageBox::customExec(dialog.get()).toString();
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
            if (!saveConfiguration(m_installations->selection(), validation)) {
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
            {QStringLiteral("transientParent"), QVariant::fromValue(parent)},
        }));
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
        m_updateCheckPending = false;
        m_installations->cancelLatestVersionRequest();
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
