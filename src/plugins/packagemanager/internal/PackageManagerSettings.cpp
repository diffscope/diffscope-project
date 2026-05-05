#include "PackageManagerSettings.h"

#include <algorithm>

#include <QDir>
#include <QSettings>
#include <QtGlobal>

#include <CoreApi/runtimeinterface.h>

namespace PackageManager {

    static PackageManagerSettings *m_instance = nullptr;

    PackageManagerSettings::PackageManagerSettings(const QString &pluginLocation, QObject *parent)
        : QObject(parent), m_pluginLocation(pluginLocation) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        load();
    }

    PackageManagerSettings::~PackageManagerSettings() {
        save();
        m_instance = nullptr;
    }

    PackageManagerSettings *PackageManagerSettings::instance() {
        return m_instance;
    }

    QString PackageManagerSettings::dspmPath() {
        Q_ASSERT(m_instance);
        return m_instance->m_dspmPath;
    }

    void PackageManagerSettings::setDspmPath(const QString &dspmPath) {
        Q_ASSERT(m_instance);
        auto normalized = QDir::toNativeSeparators(dspmPath.trimmed());
        if (m_instance->m_dspmPath == normalized)
            return;
        m_instance->m_dspmPath = normalized;
        Q_EMIT m_instance->dspmPathChanged();
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

    QString PackageManagerSettings::defaultDspmPath() {
#if defined(Q_OS_WIN)
        const auto executableName = QStringLiteral("dspm.exe");
#else
        const auto executableName = QStringLiteral("dspm");
#endif
        if (m_instance) {
            return QDir::toNativeSeparators(QDir(m_instance->m_pluginLocation).filePath(QStringLiteral("dspm/") + executableName));
        }
        return executableName;
    }

    void PackageManagerSettings::load() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        m_dspmPath = settings->value("dspmPath", defaultDspmPath()).toString();
        m_packageDir = settings->value("packageDir", defaultPackageDir()).toString();
        m_timeoutSeconds = std::max(1, settings->value("timeoutSeconds", 5).toInt());
        settings->endGroup();

        Q_EMIT dspmPathChanged();
        Q_EMIT packageDirChanged();
        Q_EMIT timeoutSecondsChanged();
    }

    void PackageManagerSettings::save() const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("dspmPath", m_dspmPath);
        settings->setValue("packageDir", m_packageDir);
        settings->setValue("timeoutSeconds", m_timeoutSeconds);
        settings->endGroup();
    }

}
