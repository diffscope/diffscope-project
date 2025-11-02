#ifndef DIFFSCOPE_MAINTENANCE_APPLICATIONUPDATECHECKER_P_H
#define DIFFSCOPE_MAINTENANCE_APPLICATIONUPDATECHECKER_P_H

#include <QNetworkAccessManager>

#include <maintenance/internal/applicationupdatechecker.h>

namespace Maintenance {

    class ApplicationUpdateCheckerPrivate {
        Q_DECLARE_PUBLIC(ApplicationUpdateChecker)
    public:
        ApplicationUpdateChecker *q_ptr;

        QNetworkAccessManager *networkManager;
        QString feedBaseUrl;
        SVS::Semver ignoredVersion;

        bool autoCheckForUpdates = true;
        ApplicationUpdateChecker::UpdateOption updateOption = ApplicationUpdateChecker::UO_Stable;

        void init();
    };

}

#endif //DIFFSCOPE_MAINTENANCE_APPLICATIONUPDATECHECKER_P_H
