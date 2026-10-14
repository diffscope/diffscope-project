// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPDOWNLOADCATALOG_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPDOWNLOADCATALOG_H

#include <QBuffer>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

#include <SVSCraftCore/Semver.h>
#include <SVSCraftNetwork/DownloadSession.h>

namespace LibreSVIPFormatConverter::Internal {

    struct LibreSVIPDownloadArtifact {
        SVS::Semver version;
        QString versionText;
        QUrl url;
    };

    class LibreSVIPDownloadCatalog : public QObject {
        Q_OBJECT
    public:
        explicit LibreSVIPDownloadCatalog(QObject *parent = nullptr);

        const QList<LibreSVIPDownloadArtifact> &artifacts() const;
        QString errorString() const;
        void start();
        void cancel();

    Q_SIGNALS:
        void progressChanged(qint64 received, qint64 total);
        void finished();

    private:
        struct Release {
            SVS::Semver version;
            QString versionText;
            QUrl manifestUrl;
        };

        void request(const QUrl &url);
        void processDocument();
        void finish();

        QBuffer m_data;
        SVS::DownloadSession m_session;
        QList<Release> m_releases;
        QList<LibreSVIPDownloadArtifact> m_artifacts;
        QString m_errorMessage;
        qsizetype m_releaseIndex{-1};
        bool m_active{};
    };

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPDOWNLOADCATALOG_H
