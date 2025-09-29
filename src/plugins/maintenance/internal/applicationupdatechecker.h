#ifndef DIFFSCOPE_MAINTENANCE_APPLICATIONUPDATECHECKER_H
#define DIFFSCOPE_MAINTENANCE_APPLICATIONUPDATECHECKER_H

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <qqmlintegration.h>

#include <SVSCraftCore/Semver.h>

class QQmlEngine;
class QJSEngine;

namespace Maintenance {

    class ApplicationUpdateCheckerPrivate;

    class ApplicationUpdateChecker : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(ApplicationUpdateChecker)

        Q_PROPERTY(bool autoCheckForUpdates READ autoCheckForUpdates WRITE setAutoCheckForUpdates NOTIFY autoCheckForUpdatesChanged)
        Q_PROPERTY(Maintenance::ApplicationUpdateChecker::UpdateOption updateOption READ updateOption WRITE setUpdateOption NOTIFY updateOptionChanged)

    public:
        struct ReleaseInfo {
            SVS::Semver version;
            bool isPreRelease;
            QString description;
            QUrl downloadLink;
            QUrl detailsLink;
        };

        enum UpdateOption {
            UO_Stable,
            UO_Beta,
        };
        Q_ENUM(UpdateOption)

        static ApplicationUpdateChecker *instance();

        static inline ApplicationUpdateChecker *create(QQmlEngine *, QJSEngine *) {
            return instance();
        }

        Q_INVOKABLE static void checkForUpdate(bool silent = false);

        static void ignoreVersion(const SVS::Semver &version);

        static bool autoCheckForUpdates();
        static void setAutoCheckForUpdates(bool autoCheckForUpdates);

        static UpdateOption updateOption();
        static void setUpdateOption(UpdateOption updateOption);

        void load();
        void save() const;

    Q_SIGNALS:
        void alreadyUpToDate();
        void showNewVersionRequested(const ReleaseInfo &releaseInfo, bool silent);
        void failed(const QString &reason, bool silent);
        void autoCheckForUpdatesChanged();
        void updateOptionChanged();

    private:
        explicit ApplicationUpdateChecker(QObject *parent = nullptr);
        ~ApplicationUpdateChecker() override;

        void getUpdateFeed(const QString &channel, bool silent);
        bool parseUpdateFeed(const QByteArray &data, ReleaseInfo &releaseInfo, QString &errorMessage);
        void handleNewVersion(const ReleaseInfo &releaseInfo, bool silent);

        QString getCurrentChannel() const;
        void saveSettings() const;
        void loadSettings();


        friend class MaintenancePlugin;
        QScopedPointer<ApplicationUpdateCheckerPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_MAINTENANCE_APPLICATIONUPDATECHECKER_H