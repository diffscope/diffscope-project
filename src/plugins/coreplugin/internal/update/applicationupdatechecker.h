#ifndef DIFFSCOPE_COREPLUGIN_APPLICATIONUPDATECHECKER_H
#define DIFFSCOPE_COREPLUGIN_APPLICATIONUPDATECHECKER_H

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <SVSCraftCore/Semver.h>

namespace Core::Internal {

    class ApplicationUpdateChecker : public QObject {
        Q_OBJECT
    public:
        struct ReleaseInfo {
            SVS::Semver version;
            bool isPreRelease;
            QString description;
            QUrl downloadLink;
            QUrl detailsLink;
        };

        static ApplicationUpdateChecker *instance();

        static void checkForUpdate(bool silent = false);

        static void ignoreVersion(const SVS::Semver &version);

    Q_SIGNALS:
        void alreadyUpToDate();
        void showNewVersionRequested(const ReleaseInfo &releaseInfo, bool silent);
        void failed(const QString &reason, bool silent);

    private:
        explicit ApplicationUpdateChecker(QObject *parent = nullptr);
        ~ApplicationUpdateChecker() override;

        void getUpdateFeed(const QString &channel, bool silent);
        bool parseUpdateFeed(const QByteArray &data, ReleaseInfo &releaseInfo, QString &errorMessage);
        void handleNewVersion(const ReleaseInfo &releaseInfo, bool silent);

        static QString getCurrentChannel();
        void saveSettings() const;
        void loadSettings();

        QNetworkAccessManager *m_networkManager;
        QString m_feedBaseUrl;
        SVS::Semver m_ignoredVersion;

        friend class CorePlugin;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_APPLICATIONUPDATECHECKER_H
