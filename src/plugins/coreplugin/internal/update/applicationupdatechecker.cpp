#include "applicationupdatechecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QApplication>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/internal/behaviorpreference.h>
#include <coreplugin/icore.h>

namespace Core::Internal {

    static ApplicationUpdateChecker *m_instance = nullptr;
    static const QString DEFAULT_FEED_BASE_URL = "https://releases.diffscope.org/feed/latest/";

    ApplicationUpdateChecker::ApplicationUpdateChecker(QObject *parent)
        : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        loadSettings();
    }

    ApplicationUpdateChecker::~ApplicationUpdateChecker() {
        saveSettings();
        m_instance = nullptr;
    }

    ApplicationUpdateChecker *ApplicationUpdateChecker::instance() {
        return m_instance;
    }

    void ApplicationUpdateChecker::checkForUpdate(bool silent) {
        Q_ASSERT(m_instance);
        QString channel = getCurrentChannel();
        m_instance->getUpdateFeed(channel, silent);
    }

    void ApplicationUpdateChecker::getUpdateFeed(const QString &channel, bool silent) {
        QString url = m_feedBaseUrl + channel;
        auto request = QNetworkRequest(QUrl(url));
        request.setHeader(QNetworkRequest::UserAgentHeader, "diffscope/1.0");
        
        auto reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, silent]() {
            reply->deleteLater();
            
            if (reply->error() != QNetworkReply::NoError) {
                emit failed(reply->errorString(), silent);
                return;
            }
            
            QByteArray data = reply->readAll();
            ReleaseInfo releaseInfo;
            QString errorMessage;
            if (parseUpdateFeed(data, releaseInfo, errorMessage)) {
                handleNewVersion(releaseInfo, silent);
            } else {
                emit failed(errorMessage, silent);
            }
        });
    }

    bool ApplicationUpdateChecker::parseUpdateFeed(const QByteArray &data, ReleaseInfo &releaseInfo, QString &errorMessage) {
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
        if (!newFeedUrl.isEmpty() && newFeedUrl != m_feedBaseUrl) {
            m_feedBaseUrl = newFeedUrl;
            saveSettings();
        }
        
        return true;
    }

    void ApplicationUpdateChecker::handleNewVersion(const ReleaseInfo &releaseInfo, bool silent) {
        auto appVersion = QApplication::applicationVersion();
        if (releaseInfo.version <= SVS::Semver(appVersion)) {
            emit alreadyUpToDate();
            return; // This version is already up to date
        }
        // Check if this version should be ignored
        if (silent && m_ignoredVersion.isValid() && releaseInfo.version <= m_ignoredVersion) {
            return; // Ignore this version
        }
        
        // Clear ignored version if it's invalid or older than the new release
        if (m_ignoredVersion.isValid() && releaseInfo.version > m_ignoredVersion) {
            m_ignoredVersion = SVS::Semver(); // Reset to invalid version
            saveSettings();
        }
        
        emit showNewVersionRequested(releaseInfo, silent);
    }

    void ApplicationUpdateChecker::ignoreVersion(const SVS::Semver &version) {
        Q_ASSERT(m_instance);
        m_instance->m_ignoredVersion = version;
        m_instance->saveSettings();
    }

    QString ApplicationUpdateChecker::getCurrentChannel() {
        if (BehaviorPreference::updateOption() == BehaviorPreference::UO_Beta) {
            return "beta";
        }
        return "stable";
    }

    void ApplicationUpdateChecker::saveSettings() const {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("feedBaseUrl", m_feedBaseUrl);
        if (m_ignoredVersion.isValid()) {
            settings->setValue("ignoredVersion", m_ignoredVersion.toString());
        } else {
            settings->remove("ignoredVersion");
        }
        settings->endGroup();
    }

    void ApplicationUpdateChecker::loadSettings() {
        auto settings = PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        m_feedBaseUrl = settings->value("feedBaseUrl", DEFAULT_FEED_BASE_URL).toString();
        QString ignoredVersionStr = settings->value("ignoredVersion", "").toString();
        if (!ignoredVersionStr.isEmpty()) {
            m_ignoredVersion = SVS::Semver(ignoredVersionStr);
        }
        settings->endGroup();
    }

}
