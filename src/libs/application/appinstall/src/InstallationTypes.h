// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_APPINSTALL_INSTALLATIONTYPES_H
#define DIFFSCOPE_APPINSTALL_INSTALLATIONTYPES_H

#include <functional>

#include <QString>
#include <QUrl>
#include <QVariant>

#include <appinstall/AppInstallGlobal.h>

class QObject;

namespace AppInstall {

    // Reports an empty string if the version is unavailable.
    using VersionResultHandler = std::function<void(const QString &version)>;
    using VersionProbeCancellation = std::function<void()>;
    // Starts a probe using an application-specific mechanism. The context owns probe resources.
    using VersionProbe = std::function<VersionProbeCancellation(const QString &executablePath, QObject *context, VersionResultHandler finished)>;

    struct APPINSTALL_EXPORT InstallationCandidate {
        QString id;
        // The directory may be renamed after validation.
        QString directory;
        QString executablePath;
        QString channel;
        QString version;
    };

    struct APPINSTALL_EXPORT InstallationOperation {
        std::function<bool()> isCancelled;
        std::function<void(const QString &)> setStatusText;
    };

    struct APPINSTALL_EXPORT InstallationValidationResult {
        bool success{};
        bool cancelled{};
        QString errorMessage;
        QVariant data;
    };

    struct APPINSTALL_EXPORT DownloadResult {
        QString id;
        QVariant validationData;
    };

    struct APPINSTALL_EXPORT ProductConfiguration {
        QString id;
        QString displayName;
        QString bundledExecutablePath;
        QString executableRelativePath;
        QString downloadedVersionsDirectory;
        // Specifies the directory containing index.json and nightly.json.
        QUrl catalogUrl;
        QString variant = QStringLiteral("portable");
        int versionTimeoutMs = 5000;
        // Returns a cleanup callback that releases installation files on completion, cancellation, or timeout.
        // An absent probe leaves the version unknown.
        VersionProbe probeVersion;
        // Validates an extracted installation before publication. Releases installation files before returning.
        std::function<InstallationValidationResult(const InstallationCandidate &, const InstallationOperation &)> validateInstallation;
    };

    struct APPINSTALL_EXPORT InstallationSelection {
        QString id;
        QVariantList localInstallations;

        bool operator==(const InstallationSelection &) const = default;
    };

}

#endif // DIFFSCOPE_APPINSTALL_INSTALLATIONTYPES_H
