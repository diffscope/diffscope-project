#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPMANAGER_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPMANAGER_H

#include <QObject>

#include <libresvipformatconverter/internal/LibreSVIPTypes.h>

class QProcess;
class QWindow;

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPManager : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString executablePath READ executablePath NOTIFY configurationChanged)
        Q_PROPERTY(bool downloadedInstallationExists READ downloadedInstallationExists NOTIFY configurationChanged)
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

        bool runPreExecCheck();
        bool browseAndConfigure(QWindow *parent, bool notifyRetry = false);
        bool downloadAndConfigure(QWindow *parent, bool notifyRetry = false);
        bool removeDownloadedInstallation(QWindow *parent);

        LibreSVIPConversionResult convert(const LibreSVIPConversionRequestData &request, QWindow *parent);

    Q_SIGNALS:
        void configurationChanged();
        void catalogChanged();

    private:
        LibreSVIPValidationResult validateExecutable(const QString &path, QWindow *parent, bool showProgress);
        LibreSVIPValidationResult validateExecutableInternal(const QString &path);
        bool cacheIsCurrent() const;
        bool saveConfiguration(const QString &path, const LibreSVIPValidationResult &result);
        void clearConfiguration();
        void loadConfiguration();

        QString pickExecutable(QWindow *parent) const;
        QString downloadedRoot() const;
        QString downloadedExecutable() const;
        QWindow *defaultParentWindow() const;

        void stopProcess(bool force = false);
        void showRetryMessage(QWindow *parent) const;
        void showError(QWindow *parent, const QString &title, const QString &message) const;

        QString m_executablePath;
        QByteArray m_cachedSha512;
        LibreSVIPPluginCatalog m_catalog;
        bool m_busy{};
        QProcess *m_process{};
    };

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPMANAGER_H
