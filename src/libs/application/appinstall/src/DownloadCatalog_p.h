// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_APPINSTALL_DOWNLOADCATALOG_P_H
#define DIFFSCOPE_APPINSTALL_DOWNLOADCATALOG_P_H

#include <array>

#include <QBuffer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariant>

#include <SVSCraftNetwork/DownloadSession.h>

namespace AppInstall {

    struct DownloadArtifact {
        QString channel;
        QString version;
        QString format;
        QUrl url;
        qint64 size{};
        QByteArray sha256;
        QUrl releaseNotes;
    };

    class DownloadCatalog : public QObject {
        Q_OBJECT
        Q_PROPERTY(QVariantList channels READ channels NOTIFY changed)
        Q_PROPERTY(QStringList versions READ versions NOTIFY changed)
        Q_PROPERTY(int channelIndex READ channelIndex WRITE setChannelIndex NOTIFY changed)
        Q_PROPERTY(int versionIndex READ versionIndex WRITE setVersionIndex NOTIFY changed)
        Q_PROPERTY(bool busy READ busy NOTIFY changed)
        Q_PROPERTY(bool available READ available NOTIFY changed)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY changed)
        Q_PROPERTY(QUrl releaseNotes READ releaseNotes NOTIFY changed)
    public:
        explicit DownloadCatalog(const QUrl &catalogUrl, const QString &variant, QObject *parent = nullptr);

        QVariantList channels() const;
        QStringList versions() const;
        int channelIndex() const;
        void setChannelIndex(int index);
        int versionIndex() const;
        void setVersionIndex(int index);
        bool busy() const;
        bool available() const;
        QString errorMessage() const;
        QUrl releaseNotes() const;
        DownloadArtifact artifact() const;
        QVariantMap latestVersions() const;
        void start();
        void cancel();
        Q_INVOKABLE void retry();

    Q_SIGNALS:
        void changed();

    private:
        struct Release {
            QString version;
            QUrl manifest;
        };
        enum class Request { Index, Nightly, Preferred, Selection };

        void request(const QUrl &url, Request kind);
        void completeRequest();
        bool readIndex(int channel);
        bool readManifest(int channel, int version);
        void findPreferredChannel();
        void loadSelection();
        void fail(const QString &message);
        QString selectionKey(int channel, int version) const;

        QUrl m_catalogUrl;
        QString m_variant;
        QBuffer m_data;
        SVS::DownloadSession m_session;
        QTimer m_timeout;
        std::array<QList<Release>, 3> m_releases;
        QHash<QString, DownloadArtifact> m_artifacts;
        QString m_errorMessage;
        Request m_request{Request::Index};
        int m_channelIndex{};
        int m_versionIndex{};
        int m_preferredIndex{};
        bool m_busy{};
        bool m_active{};
        bool m_notFound{};
        bool m_initialized{};
        bool m_catalogFound{};
    };

}

#endif // DIFFSCOPE_APPINSTALL_DOWNLOADCATALOG_P_H
