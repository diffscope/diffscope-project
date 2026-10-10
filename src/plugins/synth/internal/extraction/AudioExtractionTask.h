// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_AUDIOEXTRACTIONTASK_H
#define DIFFSCOPE_SYNTH_AUDIOEXTRACTIONTASK_H

#include <atomic>
#include <memory>

#include <QFuture>
#include <QFutureWatcher>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariantList>
#include <qqmlintegration.h>

#include <synth/internal/ApiClient.h>
#include <synth/internal/ExtractionAudioCodec.h>

class QNetworkAccessManager;
class QNetworkReply;

namespace Synth::Internal {

    struct ExtractionNote {
        double startSeconds{};
        double durationSeconds{};
        int cent{};
    };

    struct ExtractionBeat {
        double seconds{};
        bool downbeat{};
    };

    struct ExtractionPitchSegment {
        double startSeconds{};
        QList<double> cents;
    };

    struct ExtractionStem {
        QString name;
        QString filePath;
        QString digest;
    };

    struct AudioExtractionResult {
        QList<ExtractionNote> notes;
        QList<ExtractionBeat> beats;
        QList<ExtractionPitchSegment> pitch;
        double pitchSampleRate{};
        QList<ExtractionStem> stems;
    };

    class AudioExtractionTask : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(bool busy READ busy NOTIFY changed)
        Q_PROPERTY(bool refreshing READ refreshing NOTIFY changed)
        Q_PROPERTY(QVariantList extractors READ extractors NOTIFY extractorsChanged)
        Q_PROPERTY(QString error READ error NOTIFY changed)
    public:
        enum Type { Notes, Tempo, Pitch, Separation };
        Q_ENUM(Type)

        explicit AudioExtractionTask(Type type, QObject *parent = nullptr);
        ~AudioExtractionTask() override;

        bool busy() const;
        bool refreshing() const;
        QVariantList extractors() const;
        QString error() const;
        const AudioExtractionResult &result() const;
        void start(int extractorIndex, const ExtractionAudioCodec::Input &input);

        Q_INVOKABLE void refresh();
        Q_INVOKABLE void cancel();

    Q_SIGNALS:
        void changed();
        void extractorsChanged();
        void succeeded();

    private:
        struct Extractor {
            ServiceInstanceConfiguration service;
            Api::V1::ExtractorInfo info;
            QStringList trackNames;
        };

        template<typename T, typename Handler>
        void watch(QFuture<Api::ApiResult<T>> future, Handler handler) {
            auto watcher = new QFutureWatcher<Api::ApiResult<T>>(this);
            const auto generation = m_generation;
            connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, generation, handler] {
                watcher->deleteLater();
                if (generation != m_generation || watcher->future().isCanceled())
                    return;
                if (watcher->future().resultCount() == 0) {
                    fail(tr("The extraction request did not return a result."));
                    return;
                }
                handler(watcher->result());
            });
            watcher->setFuture(future);
        }

        void fail(const QString &message);
        void prepare(const Extractor &extractor, const ExtractionAudioCodec::Input &input);
        void request(const Extractor &extractor, const QString &audioUrl);
        void downloadNextStem();
        void download(const QUrl &url, int redirects = 0);
        void convertStem(const QByteArray &bytes);
        void complete();

        Type m_type;
        Api::ApiClient m_client;
        QNetworkAccessManager *m_network{};
        QPointer<QNetworkReply> m_reply;
        QFuture<ExtractionAudioCodec::Result> m_conversion;
        std::shared_ptr<std::atomic_bool> m_cancelled;
        QTemporaryDir m_temporaryDirectory;
        QList<Extractor> m_extractors;
        Extractor m_activeExtractor;
        AudioExtractionResult m_result;
        QList<Api::V1::SeparationTrack> m_stems;
        QString m_error;
        QStringList m_refreshErrors;
        int m_pendingMetadata{};
        quint64 m_generation{};
        bool m_busy{};
    };

}

#endif // DIFFSCOPE_SYNTH_AUDIOEXTRACTIONTASK_H
