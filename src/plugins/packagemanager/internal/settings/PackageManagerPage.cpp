// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageManagerPage.h"

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QScopedValueRollback>

#include <CoreApi/runtimeinterface.h>

#include <appinstall/InstallationManager.h>

#include <packagemanager/internal/PackageManagerSettings.h>
#include <packagemanager/internal/PackageManagerUtils.h>

namespace PackageManager {

    Q_STATIC_LOGGING_CATEGORY(lcPackageManagerPage, "diffscope.packagemanager.packagemanagerpage")

    namespace {

        QString urlToLocalFile(const QUrl &url) {
            return url.isLocalFile() ? AppInstall::InstallationManager::normalizedPath(url.toLocalFile()) : QString{};
        }

    }

    PackageManagerPage::PackageManagerPage(QObject *parent)
        : Core::ISettingPage("org.diffscope.packagemanager.PackageManager", parent) {
        setTitle(tr("Package Manager"));
        setDescription(tr("Configure package manager settings"));
        connect(PackageManagerSettings::instance()->installationManager(), &AppInstall::InstallationManager::installationsChanged, this, &PackageManagerPage::installationsChanged);
        connect(PackageManagerSettings::instance()->installationManager(), &AppInstall::InstallationManager::busyChanged, this, &PackageManagerPage::busyChanged);
    }

    PackageManagerPage::~PackageManagerPage() {
        delete m_widget;
    }

    bool PackageManagerPage::matches(const QString &word) {
        return Core::ISettingPage::matches(word) || widgetMatches(word);
    }

    QString PackageManagerPage::sortKeyword() const {
        return QStringLiteral("Package Manager");
    }

    QObject *PackageManagerPage::widget() {
        if (m_widget)
            return m_widget;

        qCDebug(lcPackageManagerPage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.PackageManager", "PackageManagerPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        if (!m_widget) {
            qFatal() << component.errorString();
        }
        m_widget->setParent(this);
        return m_widget;
    }

    void PackageManagerPage::beginSetting() {
        qCInfo(lcPackageManagerPage) << "Beginning setting";
        widget();
        m_selection = PackageManagerSettings::instance()->installationManager()->selection();
        setErrorMessage({});
        Q_EMIT installationsChanged();
        Q_EMIT installationChanged();
        PackageManagerSettings::instance()->installationManager()->refreshVersions(m_selection);
        m_widget->setProperty("packageDir", PackageManagerSettings::instance()->property("packageDir"));
        m_widget->setProperty("timeoutSeconds", PackageManagerSettings::instance()->property("timeoutSeconds"));
        m_widget->setProperty("started", true);
        Core::ISettingPage::beginSetting();
    }

    bool PackageManagerPage::accept() {
        qCInfo(lcPackageManagerPage) << "Accepting";
        if (busy())
            return false;
        PackageManagerSettings::instance()->installationManager()->setSelection(m_selection);
        PackageManagerSettings::instance()->setProperty("packageDir", m_widget->property("packageDir"));
        PackageManagerSettings::instance()->setProperty("timeoutSeconds", m_widget->property("timeoutSeconds"));
        PackageManagerSettings::instance()->save();
        return Core::ISettingPage::accept();
    }

    void PackageManagerPage::endSetting() {
        qCInfo(lcPackageManagerPage) << "Ending setting";
        m_widget->setProperty("started", false);
        Core::ISettingPage::endSetting();
    }

    QVariantList PackageManagerPage::installations() const {
        return PackageManagerSettings::instance()->installationManager()->installations(m_selection);
    }

    QString PackageManagerPage::installationId() const {
        return m_selection.id;
    }

    void PackageManagerPage::setInstallationId(const QString &id) {
        if (m_selection.id == id)
            return;
        m_selection.id = id;
        setErrorMessage({});
        Q_EMIT installationChanged();
        markDirty();
    }

    QString PackageManagerPage::executablePath() const {
        return PackageManagerSettings::instance()->installationManager()->executablePath(m_selection);
    }

    QString PackageManagerPage::errorMessage() const {
        return m_errorMessage;
    }

    bool PackageManagerPage::busy() const {
        return m_validating || PackageManagerSettings::instance()->installationManager()->busy();
    }

    void PackageManagerPage::addLocalInstallation(const QUrl &url) {
        if (busy())
            return;
        const auto notify = qScopeGuard([this] { Q_EMIT busyChanged(); });
        QScopedValueRollback guard(m_validating, true);
        Q_EMIT busyChanged();
        const auto path = urlToLocalFile(url);
        const auto result = validateDspmExecutable(path, m_widget->property("timeoutSeconds").toInt(), [this] { return !m_widget->property("started").toBool(); });
        if (!result.ok) {
            setErrorMessage(result.errorMessage);
            return;
        }
        auto manager = PackageManagerSettings::instance()->installationManager();
        QString error;
        if (!manager->addLocalInstallation(m_selection, path, &error)) {
            setErrorMessage(error);
            return;
        }
        setErrorMessage({});
        Q_EMIT installationsChanged();
        Q_EMIT installationChanged();
        markDirty();
        manager->refreshVersions(m_selection);
    }

    void PackageManagerPage::removeLocalInstallation() {
        if (busy())
            return;
        if (!PackageManagerSettings::instance()->installationManager()->removeLocalInstallation(m_selection))
            return;
        setErrorMessage({});
        Q_EMIT installationsChanged();
        Q_EMIT installationChanged();
        markDirty();
    }

    void PackageManagerPage::downloadInstallation() {
        if (busy())
            return;
        auto item = qobject_cast<QQuickItem *>(widget());
        const auto result = PackageManagerSettings::instance()->installationManager()->download(item ? item->window() : nullptr);
        if (!result.id.isEmpty())
            setInstallationId(result.id);
    }

    void PackageManagerPage::uninstallInstallation() {
        if (busy() || !m_selection.id.startsWith(QStringLiteral("downloaded:")))
            return;
        auto manager = PackageManagerSettings::instance()->installationManager();
        auto item = qobject_cast<QQuickItem *>(widget());
        if (manager->uninstall(m_selection.id, item ? item->window() : nullptr))
            setInstallationId(manager->defaultInstallationId());
    }

    void PackageManagerPage::setErrorMessage(const QString &message) {
        if (m_errorMessage == message)
            return;
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }

    QString PackageManagerPage::localFilePath(const QUrl &url) {
        return urlToLocalFile(url);
    }

    bool PackageManagerPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
