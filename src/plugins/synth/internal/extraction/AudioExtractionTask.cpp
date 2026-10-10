// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioExtractionTask.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QFile>
#include <QLocale>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QVariantMap>
#include <QtConcurrent/QtConcurrentRun>

#include <synth/internal/SynthService.h>

#include <xxhash.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAudioExtractionTask, "diffscope.synth.extraction.task")

    namespace {

        bool sameOrigin(const QUrl &left, const QUrl &right) {
            const auto port = [](const QUrl &url) { return url.port(url.scheme() == QStringLiteral("https") ? 443 : 80); };
            return left.scheme() == right.scheme() && left.host() == right.host() && port(left) == port(right);
        }

    }

    AudioExtractionTask::AudioExtractionTask(Type type, QObject *parent)
        : QObject(parent), m_type(type), m_client(this), m_network(new QNetworkAccessManager(this)),
          m_cancelled(std::make_shared<std::atomic_bool>(false)) {
    }

    AudioExtractionTask::~AudioExtractionTask() {
        cancel();
        m_conversion.waitForFinished();
    }

    bool AudioExtractionTask::busy() const { return m_busy; }
    bool AudioExtractionTask::refreshing() const { return m_pendingMetadata > 0; }
    QString AudioExtractionTask::error() const { return m_error; }
    const AudioExtractionResult &AudioExtractionTask::result() const { return m_result; }

    QVariantList AudioExtractionTask::extractors() const {
        QVariantList result;
        for (const auto &extractor : m_extractors) {
            result.append(QVariantMap{
                {QStringLiteral("name"), tr("%1 — %2").arg(extractor.info.name, extractor.service.name())},
                {QStringLiteral("trackLayout"), extractor.trackNames.join(tr(", "))},
            });
        }
        return result;
    }

    void AudioExtractionTask::refresh() {
        if (m_busy)
            return;
        cancel();
        m_extractors.clear();
        Q_EMIT extractorsChanged();
        m_error.clear();
        m_refreshErrors.clear();
        const auto services = SynthService::instance()->serviceConfigurations();
        for (const auto &service : services) {
            if (service.isEnabled())
                ++m_pendingMetadata;
        }
        for (const auto &service : services) {
            if (!service.isEnabled())
                continue;
            watch(m_client.getExtractors(service, QLocale().bcp47Name()), [this, service](const auto &result) {
                if (result.hasError()) {
                    m_refreshErrors.append(tr("%1: %2").arg(service.name(), result.error().message));
                } else {
                    const auto &list = result.value();
                    if (m_type == Separation) {
                        for (const auto &item : list.separation) {
                            if (item.id.isEmpty())
                                continue;
                            QStringList trackNames;
                            for (const auto &track : item.tracks)
                                trackNames.append(track.name);
                            m_extractors.append({service, item, trackNames});
                        }
                    } else {
                        const auto &items = m_type == Notes ? list.note : m_type == Tempo ? list.tempo : list.pitch;
                        for (const auto &item : items) {
                            if (!item.id.isEmpty())
                                m_extractors.append({service, item, {}});
                        }
                    }
                    Q_EMIT extractorsChanged();
                }
                --m_pendingMetadata;
                m_error = m_refreshErrors.join(u'\n');
                if (m_pendingMetadata == 0 && m_extractors.isEmpty() && m_error.isEmpty())
                    m_error = tr("No extractors are available. Check your synthesis services.");
                Q_EMIT changed();
            });
        }
        if (m_pendingMetadata == 0)
            m_error = tr("No enabled synthesis services are available.");
        Q_EMIT changed();
    }

    void AudioExtractionTask::cancel() {
        ++m_generation;
        m_cancelled->store(true);
        m_client.cancelAll();
        if (m_reply)
            m_reply->abort();
        m_pendingMetadata = 0;
        m_busy = false;
        qCDebug(lcAudioExtractionTask) << "Entered idle state";
        Q_EMIT changed();
    }

    void AudioExtractionTask::fail(const QString &message) {
        qCWarning(lcAudioExtractionTask) << "Extraction failed";
        cancel();
        m_error = message.isEmpty() ? tr("The extraction request failed.") : message;
        Q_EMIT changed();
    }

    void AudioExtractionTask::start(int extractorIndex, const ExtractionAudioCodec::Input &input) {
        if (m_busy || refreshing() || extractorIndex < 0 || extractorIndex >= m_extractors.size())
            return;
        m_conversion.waitForFinished();
        ++m_generation;
        m_cancelled = std::make_shared<std::atomic_bool>(false);
        m_busy = true;
        m_error.clear();
        m_result = {};
        m_stems.clear();
        m_activeExtractor = m_extractors.at(extractorIndex);
        Q_EMIT changed();
        qCDebug(lcAudioExtractionTask) << "Entered negotiating state";
        const auto &service = m_activeExtractor.service;
        const auto &id = m_activeExtractor.info.id;
        const auto language = QLocale().bcp47Name();
        const auto handleMetadata = [this, input](const auto &result) {
            if (result.hasError()) {
                fail(result.error().message);
                return;
            }
            if (result.value().id != m_activeExtractor.info.id) {
                fail(tr("The service returned inconsistent extractor metadata."));
                return;
            }
            m_activeExtractor.info = result.value();
            prepare(m_activeExtractor, input);
        };
        if (m_type == Separation) {
            watch(m_client.getSeparationExtractor(service, id, language), handleMetadata);
        } else {
            auto future = m_type == Notes ? m_client.getNoteExtractor(service, id, language)
                : m_type == Tempo ? m_client.getTempoExtractor(service, id, language)
                : m_client.getPitchExtractor(service, id, language);
            watch(future, handleMetadata);
        }
    }

    void AudioExtractionTask::prepare(const Extractor &extractor, const ExtractionAudioCodec::Input &input) {
        qCDebug(lcAudioExtractionTask) << "Entered preparing state";
        auto watcher = new QFutureWatcher<ExtractionAudioCodec::Result>(this);
        const auto generation = m_generation;
        connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, generation, extractor] {
            watcher->deleteLater();
            if (generation != m_generation)
                return;
            const auto result = watcher->result();
            if (result.bytes.isEmpty()) {
                fail(result.error);
                return;
            }
            request(extractor, QStringLiteral("data:%1;base64,%2").arg(result.mimeType, QString::fromLatin1(result.bytes.toBase64())));
        });
        m_conversion = QtConcurrent::run([input, info = extractor.info, cancelled = m_cancelled] {
            return ExtractionAudioCodec::encode(input, info, cancelled);
        });
        watcher->setFuture(m_conversion);
    }

    void AudioExtractionTask::request(const Extractor &extractor, const QString &audioUrl) {
        qCDebug(lcAudioExtractionTask) << "Entered extracting state";
        const Api::V1::ExtractionRequest request{extractor.info.id, {audioUrl}};
        if (m_type == Notes) {
            watch(m_client.extractNotes(extractor.service, request), [this](const auto &result) {
                if (result.hasError()) {
                    fail(result.error().message);
                    return;
                }
                double end = 0;
                for (const auto &note : result.value().output.notes) {
                    const double start = end + note.position.gap;
                    end = start + note.position.duration;
                    if (!std::isfinite(start) || !std::isfinite(end) || start < 0 || note.position.duration <= 0
                        || note.cent < 0 || note.cent > 12800) {
                        fail(tr("The extractor returned invalid note data."));
                        return;
                    }
                    m_result.notes.append({start, note.position.duration, note.cent});
                }
                complete();
            });
        } else if (m_type == Tempo) {
            watch(m_client.extractTempo(extractor.service, request), [this](const auto &result) {
                if (result.hasError()) {
                    fail(result.error().message);
                    return;
                }
                double previous = -1;
                for (const auto &beat : result.value().output.beats) {
                    if (!std::isfinite(beat.position) || beat.position < 0 || beat.position < previous) {
                        fail(tr("The extractor returned invalid beat data."));
                        return;
                    }
                    m_result.beats.append({beat.position, beat.downbeat});
                    previous = beat.position;
                }
                complete();
            });
        } else if (m_type == Pitch) {
            watch(m_client.extractPitch(extractor.service, request), [this](const auto &result) {
                if (result.hasError()) {
                    fail(result.error().message);
                    return;
                }
                const auto &output = result.value().output;
                if (!std::isfinite(output.sampleRate) || output.sampleRate <= 0) {
                    fail(tr("The extractor returned invalid pitch data."));
                    return;
                }
                m_result.pitchSampleRate = output.sampleRate;
                double position = 0;
                for (const auto &segment : output.segments) {
                    if (segment.gap < 0 || std::ranges::any_of(segment.pitch, [](double value) { return !std::isfinite(value); })) {
                        fail(tr("The extractor returned invalid pitch data."));
                        return;
                    }
                    position += segment.gap;
                    m_result.pitch.append({position / output.sampleRate, segment.pitch});
                    position += segment.pitch.size();
                }
                complete();
            });
        } else {
            Api::V1::SeparationExtractionRequest separation;
            separation.extractor = extractor.info.id;
            separation.input.audioUrl = audioUrl;
            // An empty format list accepts any server output format; transport is explicit.
            separation.acceptableSchemes = {QStringLiteral("data"), QStringLiteral("http"), QStringLiteral("https")};
            watch(m_client.extractSeparation(extractor.service, separation, QLocale().bcp47Name()), [this](const auto &result) {
                if (result.hasError()) {
                    fail(result.error().message);
                    return;
                }
                m_stems = result.value().output.tracks;
                if (m_stems.isEmpty() || !m_temporaryDirectory.isValid()) {
                    fail(tr("The extractor returned no separated audio tracks."));
                    return;
                }
                downloadNextStem();
            });
        }
    }

    void AudioExtractionTask::downloadNextStem() {
        if (m_stems.isEmpty()) {
            complete();
            return;
        }
        qCDebug(lcAudioExtractionTask) << "Entered receiving state";
        const auto location = m_stems.first().audioUrl;
        if (location.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive)) {
            const auto comma = location.indexOf(u',');
            if (comma < 5) {
                fail(tr("The extractor returned an invalid audio data URL."));
                return;
            }
            const auto parts = location.mid(5, comma - 5).split(u';');
            auto bytes = QByteArray::fromPercentEncoding(location.mid(comma + 1).toLatin1());
            if (parts.contains(QStringLiteral("base64"), Qt::CaseInsensitive)) {
                const auto decoded = QByteArray::fromBase64Encoding(bytes, QByteArray::AbortOnBase64DecodingErrors);
                if (!decoded) {
                    fail(tr("The extractor returned an invalid audio data URL."));
                    return;
                }
                bytes = decoded.decoded;
            }
            convertStem(bytes);
        } else {
            download(QUrl(location));
        }
    }

    void AudioExtractionTask::download(const QUrl &url, int redirects) {
        if (!url.isValid() || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))
            || !url.userInfo().isEmpty() || redirects > 5) {
            fail(tr("The extractor returned an unsupported audio URL."));
            return;
        }
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
        const auto &service = m_activeExtractor.service;
        request.setTransferTimeout(static_cast<int>(std::clamp<qint64>(static_cast<qint64>(service.requestTimeoutSeconds()) * 1000, 1, std::numeric_limits<int>::max())));
        if (sameOrigin(url, service.baseUrl())) {
            if (service.authenticationEnabled())
                request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + service.apiKey().toUtf8());
            const auto headers = service.parsedCustomHeaders();
            for (auto it = headers.cbegin(); it != headers.cend(); ++it)
                request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
#if QT_CONFIG(ssl)
            if (!service.verifySslCertificate()) {
                auto ssl = request.sslConfiguration();
                ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
                request.setSslConfiguration(ssl);
            }
#endif
        }
        auto reply = m_network->get(request);
        m_reply = reply;
        const auto generation = m_generation;
        connect(reply, &QNetworkReply::finished, this, [this, reply, generation, redirects] {
            reply->deleteLater();
            if (generation != m_generation)
                return;
            const auto redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (!redirect.isEmpty()) {
                const auto target = reply->url().resolved(redirect);
                if (reply->url().scheme() == QStringLiteral("https") && target.scheme() != QStringLiteral("https")) {
                    fail(tr("The audio download redirected to an insecure URL."));
                    return;
                }
                download(target, redirects + 1);
                return;
            }
            const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
                fail(tr("Could not download separated audio: %1").arg(reply->errorString()));
                return;
            }
            convertStem(reply->readAll());
        });
    }

    void AudioExtractionTask::convertStem(const QByteArray &bytes) {
        qCDebug(lcAudioExtractionTask) << "Entered converting state";
        auto watcher = new QFutureWatcher<ExtractionAudioCodec::Result>(this);
        const auto generation = m_generation;
        connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, generation] {
            watcher->deleteLater();
            if (generation != m_generation)
                return;
            const auto result = watcher->result();
            if (result.bytes.isEmpty()) {
                fail(result.error);
                return;
            }
            const auto path = m_temporaryDirectory.filePath(QString::number(m_result.stems.size()) + QStringLiteral(".wav"));
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly) || file.write(result.bytes) != result.bytes.size() || !file.flush()) {
                fail(tr("Could not store separated audio."));
                return;
            }
            file.close();
            const auto digest = XXH3_128bits(result.bytes.constData(), static_cast<size_t>(result.bytes.size()));
            m_result.stems.append({m_stems.takeFirst().name, path,
                QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(&digest), sizeof(digest)).toBase64(QByteArray::Base64UrlEncoding))});
            downloadNextStem();
        });
        m_conversion = QtConcurrent::run([bytes, cancelled = m_cancelled] { return ExtractionAudioCodec::decode(bytes, cancelled); });
        watcher->setFuture(m_conversion);
    }

    void AudioExtractionTask::complete() {
        m_busy = false;
        qCDebug(lcAudioExtractionTask) << "Entered completed state";
        Q_EMIT changed();
        Q_EMIT succeeded();
    }

}

#include "moc_AudioExtractionTask.cpp"
