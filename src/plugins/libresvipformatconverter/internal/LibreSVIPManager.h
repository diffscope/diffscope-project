// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPMANAGER_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPMANAGER_H

#include <functional>

#include <QObject>

#include <libresvipformatconverter/internal/LibreSVIPTypes.h>

class QProcess;
class QBuffer;
class QWindow;

namespace Core {
    class NotificationMessage;
}

namespace SVS {
    class DownloadSession;
}

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPManager : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString executablePath READ executablePath NOTIFY configurationChanged)
        Q_PROPERTY(bool downloadedInstallationExists READ downloadedInstallationExists NOTIFY configurationChanged)
        Q_PROPERTY(bool autoCheckForUpdates READ autoCheckForUpdates WRITE setAutoCheckForUpdates
                       NOTIFY autoCheckForUpdatesChanged)
    public:
        static constexpr const char *settingsPageId() {
            return "org.diffscope.libresvipformatconverter.General";
        }

        explicit LibreSVIPManager(QObject *parent = nullptr);
        ~LibreSVIPManager() override;

        static LibreSVIPManager *instance();

        QString executablePath() const;
        const LibreSVIPPluginCatalog &catalog() const;
        bool hasCurrentCatalog() const;
        bool downloadedInstallationExists() const;
        bool autoCheckForUpdates() const;
        void setAutoCheckForUpdates(bool enabled);

        bool runPreExecCheck();
        void checkForUpdates();
        bool browseAndConfigure(QWindow *parent, bool notifyRetry = false);
        bool downloadAndConfigure(QWindow *parent, bool notifyRetry = false);
        bool clearExecutablePath(QWindow *parent);
        bool removeDownloadedInstallation(QWindow *parent);

        LibreSVIPConversionResult convert(const LibreSVIPConversionRequestData &request, QWindow *parent);

    Q_SIGNALS:
        void configurationChanged();
        void catalogChanged();
        void autoCheckForUpdatesChanged(bool enabled);

    private:
        LibreSVIPValidationResult validateExecutable(const QString &path, QWindow *parent, bool showProgress);
        LibreSVIPValidationResult validateExecutableInternal(const QString &path,
                                                              const std::function<bool()> &isCancelled = {});
        bool cacheIsCurrent() const;
        bool saveConfiguration(const QString &path, const LibreSVIPValidationResult &result);
        void clearConfiguration();
        void loadConfiguration();

        QString pickExecutable(QWindow *parent) const;
        QString downloadedRoot() const;
        QString downloadedExecutable() const;
        QString downloadedVersion() const;
        QWindow *defaultParentWindow() const;

        void stopProcess(bool force = false);
        void cancelUpdateCheck();
        void dismissUpdateNotification();
        void showUpdateNotification(const QString &latestVersion, const QString &installedVersion);
        void showRetryMessage(QWindow *parent) const;
        void showError(QWindow *parent, const QString &title, const QString &message) const;

        QString m_executablePath;
        QByteArray m_cachedSha512;
        QString m_cachedLocaleName;
        LibreSVIPPluginCatalog m_catalog;
        bool m_autoCheckForUpdates{true};
        bool m_busy{};
        QProcess *m_process{};
        QBuffer *m_updateCatalogData{};
        SVS::DownloadSession *m_updateCheckSession{};
        Core::NotificationMessage *m_updateNotification{};
    };

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPMANAGER_H
