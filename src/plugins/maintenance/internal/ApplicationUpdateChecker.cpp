#include "ApplicationUpdateChecker.h"
#include "ApplicationUpdateChecker_p.h"

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTimer>

#include <CoreApi/runtimeinterface.h>

#include <maintenance/internal/maintenanceplugin.h>

namespace Maintenance {

    static ApplicationUpdateChecker *m_instance = nullptr;
    static const QString DEFAULT_FEED_BASE_URL = "https://releases.diffscope.org/feed/latest/";

    void ApplicationUpdateCheckerPrivate::init() {
        Q_Q(ApplicationUpdateChecker);
        networkManager = new QNetworkAccessManager(q);
        feedBaseUrl = DEFAULT_FEED_BASE_URL;
        q->loadSettings();
    }

    ApplicationUpdateChecker::ApplicationUpdateChecker(QObject *parent)
        : QObject(parent), d_ptr(new ApplicationUpdateCheckerPrivate) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        Q_D(ApplicationUpdateChecker);
        d->q_ptr = this;
        d->init();
    }

    ApplicationUpdateChecker::~ApplicationUpdateChecker() {
        save();
        m_instance = nullptr;
    }

    ApplicationUpdateChecker *ApplicationUpdateChecker::instance() {
        return m_instance;
    }

    void ApplicationUpdateChecker::checkForUpdate(bool silent) {
        Q_ASSERT(m_instance);
        QString channel = m_instance->getCurrentChannel();
        m_instance->getUpdateFeed(channel, silent);
    }

    void ApplicationUpdateChecker::getUpdateFeed(const QString &channel, bool silent) {
        Q_D(ApplicationUpdateChecker);
        QString url = d->feedBaseUrl + channel;
        auto request = QNetworkRequest(QUrl(url));
        request.setHeader(QNetworkRequest::UserAgentHeader, "diffscope/1.0");

        auto reply = d->networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, silent]() {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                Q_EMIT failed(reply->errorString(), silent);
                return;
            }

            QByteArray data = reply->readAll();
            ReleaseInfo releaseInfo;
            QString errorMessage;
            if (parseUpdateFeed(data, releaseInfo, errorMessage)) {
                handleNewVersion(releaseInfo, silent);
            } else {
                Q_EMIT failed(errorMessage, silent);
            }
        });
    }

    bool ApplicationUpdateChecker::parseUpdateFeed(const QByteArray &data, ReleaseInfo &releaseInfo, QString &errorMessage) {
        Q_D(ApplicationUpdateChecker);
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            errorMessage = "Parse error";
            return false;
        }

        if (!doc.isObject()) {
            errorMessage = "Parse error";
            return false;
        }

        QJsonObject root = doc.object();
        QJsonObject releaseInfoObj = root.value("releaseInfo").toObject();

        if (releaseInfoObj.isEmpty()) {
            errorMessage = "Parse error";
            return false;
        }

        releaseInfo.version = SVS::Semver(releaseInfoObj.value("version").toString());
        releaseInfo.isPreRelease = releaseInfoObj.value("preRelease").toBool();
        releaseInfo.description = releaseInfoObj.value("description").toString();
        releaseInfo.downloadLink = QUrl(releaseInfoObj.value("downloadLink").toString());
        releaseInfo.detailsLink = QUrl(releaseInfoObj.value("detailsLink").toString());

        if (!releaseInfo.version.isValid()) {
            errorMessage = "Parse error";
            return false;
        }

        // Update feed URL if provided
        QString newFeedUrl = root.value("feedUrl").toString();
        if (!newFeedUrl.isEmpty() && newFeedUrl != d->feedBaseUrl) {
            d->feedBaseUrl = newFeedUrl;
            saveSettings();
        }

        return true;
    }

    void ApplicationUpdateChecker::handleNewVersion(const ReleaseInfo &releaseInfo, bool silent) {
        Q_D(ApplicationUpdateChecker);
        auto appVersion = QApplication::applicationVersion();
        if (releaseInfo.version <= SVS::Semver(appVersion)) {
            Q_EMIT alreadyUpToDate();
            return; // This version is already up to date
        }
        // Check if this version should be ignored
        if (silent && d->ignoredVersion.isValid() && releaseInfo.version <= d->ignoredVersion) {
            return; // Ignore this version
        }

        // Clear ignored version if it's invalid or older than the new release
        if (d->ignoredVersion.isValid() && releaseInfo.version > d->ignoredVersion) {
            d->ignoredVersion = SVS::Semver(); // Reset to invalid version
            saveSettings();
        }

        Q_EMIT showNewVersionRequested(releaseInfo, silent);
    }

    void ApplicationUpdateChecker::ignoreVersion(const SVS::Semver &version) {
        Q_ASSERT(m_instance);
        m_instance->d_func()->ignoredVersion = version;
        m_instance->saveSettings();
    }

    QString ApplicationUpdateChecker::getCurrentChannel() const {
        Q_D(const ApplicationUpdateChecker);
        if (d->updateOption == UO_Beta) {
            return "beta";
        }
        return "stable";
    }

    bool ApplicationUpdateChecker::autoCheckForUpdates() {
        Q_ASSERT(m_instance);
        return m_instance->d_func()->autoCheckForUpdates;
    }

    void ApplicationUpdateChecker::setAutoCheckForUpdates(bool autoCheckForUpdates) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        if (d->autoCheckForUpdates == autoCheckForUpdates)
            return;
        d->autoCheckForUpdates = autoCheckForUpdates;
        Q_EMIT m_instance->autoCheckForUpdatesChanged();
    }

    ApplicationUpdateChecker::UpdateOption ApplicationUpdateChecker::updateOption() {
        Q_ASSERT(m_instance);
        return m_instance->d_func()->updateOption;
    }

    void ApplicationUpdateChecker::setUpdateOption(UpdateOption updateOption) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        if (d->updateOption == updateOption)
            return;
        d->updateOption = updateOption;
        Q_EMIT m_instance->updateOptionChanged();
    }

    void ApplicationUpdateChecker::load() {
        loadSettings();
    }

    void ApplicationUpdateChecker::save() const {
        saveSettings();
    }

    void ApplicationUpdateChecker::saveSettings() const {
        Q_D(const ApplicationUpdateChecker);
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("feedBaseUrl", d->feedBaseUrl);
        settings->setValue("autoCheckForUpdates", d->autoCheckForUpdates);
        settings->setValue("updateOption", static_cast<int>(d->updateOption));
        if (d->ignoredVersion.isValid()) {
            settings->setValue("ignoredVersion", d->ignoredVersion.toString());
        } else {
            settings->remove("ignoredVersion");
        }
        settings->endGroup();
    }

    void ApplicationUpdateChecker::loadSettings() {
        Q_D(ApplicationUpdateChecker);
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        d->feedBaseUrl = settings->value("feedBaseUrl", DEFAULT_FEED_BASE_URL).toString();
        d->autoCheckForUpdates = settings->value("autoCheckForUpdates", true).toBool();
        d->updateOption = settings->value("updateOption", UO_Stable).value<UpdateOption>();
        QString ignoredVersionStr = settings->value("ignoredVersion", "").toString();
        if (!ignoredVersionStr.isEmpty()) {
            d->ignoredVersion = SVS::Semver(ignoredVersionStr);
        }
        settings->endGroup();

        Q_EMIT autoCheckForUpdatesChanged();
        Q_EMIT updateOptionChanged();
    }

}
