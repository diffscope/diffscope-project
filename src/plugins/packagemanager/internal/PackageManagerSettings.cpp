// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageManagerSettings.h"

#include <algorithm>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QSettings>
#include <QUrl>
#include <QtGlobal>

#include <CoreApi/runtimeinterface.h>

#include <appinstall/InstallationManager.h>

#include <packagemanager/internal/PackageManagerUtils.h>

namespace PackageManager {

    static PackageManagerSettings *m_instance = nullptr;

    PackageManagerSettings::PackageManagerSettings(const QString &pluginLocation, QObject *parent)
        : QObject(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        load();

        AppInstall::ProductConfiguration product;
        product.id = QStringLiteral("dspm");
        product.displayName = tr("DiffScope Package Manager");
        product.variant = QStringLiteral("standalone");
#if defined(Q_OS_WIN)
        product.executableRelativePath = QStringLiteral("dspm.exe");
#else
        product.executableRelativePath = QStringLiteral("dspm");
#endif
        product.bundledExecutablePath = QDir(pluginLocation).filePath(QStringLiteral("dspm/") + product.executableRelativePath);
        product.catalogUrl = QUrl(QString::fromUtf8(DIFFSCOPE_DSPM_CATALOG_URL));
        product.probeVersion = [](const QString &path, QObject *context, AppInstall::VersionResultHandler finished) -> AppInstall::VersionProbeCancellation {
            auto process = new QProcess(context);
            process->setStandardErrorFile(QProcess::nullDevice());
            QObject::connect(process, &QProcess::errorOccurred, context, [finished](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart)
                    finished({});
            });
            QObject::connect(process, &QProcess::finished, context, [process, finished](int code, QProcess::ExitStatus status) {
                const auto object = QJsonDocument::fromJson(process->readAllStandardOutput()).object();
                const auto version = object.value(QStringLiteral("version")).toString();
                finished(code == 0 && status == QProcess::NormalExit && object.value(QStringLiteral("name")).toString() == QStringLiteral("dspm") ? version : QString{});
            });
            process->start(path, {QStringLiteral("--json"), QStringLiteral("--version")}, QIODevice::ReadOnly);
            return [guard = QPointer<QProcess>(process)] {
                if (!guard)
                    return;
                guard->disconnect();
                if (guard->state() != QProcess::NotRunning) {
                    guard->kill();
                    guard->waitForFinished(1000);
                }
                delete guard.data();
            };
        };
        product.validateInstallation = [](const AppInstall::InstallationCandidate &candidate, const AppInstall::InstallationOperation &operation) {
            const auto result = validateDspmExecutable(candidate.executablePath, timeoutSeconds(), operation.isCancelled);
            return AppInstall::InstallationValidationResult{result.ok, result.cancelled, result.errorMessage, {}};
        };
        m_installations = new AppInstall::InstallationManager(product, this);
        connect(m_installations, &AppInstall::InstallationManager::selectionChanged, this, &PackageManagerSettings::dspmPathChanged);
    }

    PackageManagerSettings::~PackageManagerSettings() {
        m_installations->shutdown();
        save();
        m_instance = nullptr;
    }

    PackageManagerSettings *PackageManagerSettings::instance() {
        return m_instance;
    }

    QString PackageManagerSettings::dspmPath() {
        Q_ASSERT(m_instance);
        return m_instance->m_installations->executablePath();
    }

    AppInstall::InstallationManager *PackageManagerSettings::installationManager() const {
        return m_installations;
    }

    QString PackageManagerSettings::packageDir() {
        Q_ASSERT(m_instance);
        return m_instance->m_packageDir;
    }

    void PackageManagerSettings::setPackageDir(const QString &packageDir) {
        Q_ASSERT(m_instance);
        auto normalized = QDir::toNativeSeparators(packageDir.trimmed());
        if (m_instance->m_packageDir == normalized)
            return;
        m_instance->m_packageDir = normalized;
        Q_EMIT m_instance->packageDirChanged();
    }

    int PackageManagerSettings::timeoutSeconds() {
        Q_ASSERT(m_instance);
        return m_instance->m_timeoutSeconds;
    }

    void PackageManagerSettings::setTimeoutSeconds(int timeoutSeconds) {
        Q_ASSERT(m_instance);
        timeoutSeconds = std::max(1, timeoutSeconds);
        if (m_instance->m_timeoutSeconds == timeoutSeconds)
            return;
        m_instance->m_timeoutSeconds = timeoutSeconds;
        Q_EMIT m_instance->timeoutSecondsChanged();
    }

    QString PackageManagerSettings::defaultPackageDir() {
#if defined(Q_OS_WIN)
        auto base = qEnvironmentVariable("LOCALAPPDATA");
        if (base.isEmpty()) {
            base = QDir::home().filePath(QStringLiteral("AppData/Local"));
        }
        return QDir::toNativeSeparators(QDir(base).filePath(QStringLiteral("OpenVPI/DiffScope_packages")));
#elif defined(Q_OS_MACOS)
        return QDir::toNativeSeparators(QDir::home().filePath(QStringLiteral("Library/Application Support/OpenVPI/DiffScope_packages")));
#else
        auto base = qEnvironmentVariable("XDG_DATA_HOME");
        if (base.isEmpty()) {
            base = QDir::home().filePath(QStringLiteral(".local/share"));
        }
        return QDir::toNativeSeparators(QDir(base).filePath(QStringLiteral("OpenVPI/DiffScope_packages")));
#endif
    }

    void PackageManagerSettings::load() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        m_packageDir = settings->value("packageDir", defaultPackageDir()).toString();
        m_timeoutSeconds = std::max(1, settings->value("timeoutSeconds", 30).toInt());
        settings->endGroup();

        Q_EMIT packageDirChanged();
        Q_EMIT timeoutSecondsChanged();
    }

    void PackageManagerSettings::save() const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("packageDir", m_packageDir);
        settings->setValue("timeoutSeconds", m_timeoutSeconds);
        settings->endGroup();
    }

}
