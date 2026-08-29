// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ApiClient.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include <QDateTime>
#include <QFutureInterface>
#include <QHash>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>

#include <synth/internal/ApiVersion.h>

namespace Synth::Internal::Api {

Q_STATIC_LOGGING_CATEGORY(lcDsspApiClient, "diffscope.synth.api")

namespace {

    enum class SynthesisCategory {
        None,
        Pronunciation,
        Phoneme,
        Duration,
        Parameter,
        Audio,
    };

    struct ServiceOptions {
        QUuid id;
        QUrl baseUrl;
        bool authenticationEnabled{};
        QString apiKey;
        int requestTimeoutSeconds{30};
        int retryCount{5};
        int taskConcurrency{4};
        int globalConcurrency{64};
        bool verifySslCertificate{true};
        QMap<QString, QString> customHeaders;
    };

    ServiceOptions optionsFrom(const ServiceInstanceConfiguration &service) {
        return ServiceOptions{service.id(),
                              service.baseUrl(),
                              service.authenticationEnabled(),
                              service.apiKey(),
                              std::max(1, service.requestTimeoutSeconds()),
                              std::max(0, service.retryCount()),
                              std::max(1, service.taskConcurrency()),
                              std::max(1, service.globalConcurrency()),
                              service.verifySslCertificate(),
                              service.parsedCustomHeaders()};
    }

    struct RawResponse {
        int httpStatusCode{};
        QByteArray rawResponse;
        QJsonValue json{QJsonValue::Undefined};
        std::optional<ApiError> error;
        QList<ApiExchange> exchanges;
    };

    struct RequestTask {
        quint64 id{};
        ServiceOptions service;
        QByteArray method;
        QString route;
        QUrlQuery query;
        QByteArray body;
        SynthesisCategory category{SynthesisCategory::None};
        ApiVersion apiVersion{ApiVersion::V1};
        int attempt{};
        QList<ApiExchange> exchanges;
        std::function<bool()> isCanceled;
        std::function<void()> reportStarted;
        std::function<void(const RawResponse &)> reportResponse;
        std::function<void()> reportCanceledAndFinished;
    };

    QString encodedPathSegment(const QString &segment) {
        return QString::fromLatin1(QUrl::toPercentEncoding(segment));
    }

    QByteArray jsonBody(const QJsonValue &value) {
        if (value.isObject())
            return QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact);
        if (value.isArray())
            return QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact);
        return {};
    }

    QJsonValue parseJsonBestEffort(const QByteArray &body) {
        QJsonParseError parseError;
        const auto value = QJsonValue::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError)
            return QJsonValue(QJsonValue::Undefined);
        return value;
    }

    QString problemDetailsText(const QJsonValue &json) {
        if (!json.isObject())
            return {};

        const auto object = json.toObject();
        const auto title = object.value(QStringLiteral("title")).toString().trimmed();
        const auto detail = object.value(QStringLiteral("detail")).toString().trimmed();
        if (title.isEmpty())
            return detail;
        if (detail.isEmpty() || detail == title)
            return title;
        return title + u'\n' + detail;
    }

    bool isRetryableHttpStatus(int status) {
        switch (status) {
        case 408:
        case 425:
        case 429:
        case 500:
        case 502:
        case 503:
        case 504:
            return true;
        default:
            return false;
        }
    }

    bool isRetryableNetworkError(QNetworkReply::NetworkError error) {
        switch (error) {
        case QNetworkReply::ConnectionRefusedError:
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::HostNotFoundError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::NetworkSessionFailedError:
        case QNetworkReply::UnknownNetworkError:
        case QNetworkReply::ProxyConnectionRefusedError:
        case QNetworkReply::ProxyConnectionClosedError:
        case QNetworkReply::ProxyNotFoundError:
        case QNetworkReply::ProxyTimeoutError:
        case QNetworkReply::UnknownProxyError:
        case QNetworkReply::InternalServerError:
        case QNetworkReply::ServiceUnavailableError:
        case QNetworkReply::UnknownServerError:
            return true;
        default:
            return false;
        }
    }

    int retryAfterMilliseconds(const QByteArray &header, int attempt) {
        constexpr int MaximumDelay = 10000;
        bool ok = false;
        const auto seconds = QString::fromLatin1(header).trimmed().toInt(&ok);
        if (ok && seconds >= 0)
            return static_cast<int>(std::min<qint64>(MaximumDelay, static_cast<qint64>(seconds) * 1000));

        const auto date = QDateTime::fromString(QString::fromLatin1(header), Qt::RFC2822Date);
        if (date.isValid()) {
            const auto delay = QDateTime::currentDateTimeUtc().msecsTo(date.toUTC());
            if (delay > 0)
                return static_cast<int>(std::min<qint64>(MaximumDelay, delay));
        }

        const int exponent = std::clamp(attempt, 0, 5);
        return std::min(MaximumDelay, 250 * (1 << exponent));
    }

    class NetworkWorker : public QObject {
        Q_OBJECT
    public:
        explicit NetworkWorker(QObject *parent = nullptr) : QObject(parent) {}

        void enqueue(const std::shared_ptr<RequestTask> &task) {
            ensureInitialized();
            if (m_shuttingDown) {
                task->reportCanceledAndFinished();
                return;
            }
            if (task->isCanceled()) {
                task->reportCanceledAndFinished();
                return;
            }
            if (task->category == SynthesisCategory::None)
                startTask(task);
            else {
                qCDebug(lcDsspApiClient) << "Queued concurrency-limited DSSP request"
                                         << "requestId=" << task->id
                                         << "method=" << task->method
                                         << "route=" << task->route;
                m_queue.append(task);
                pumpQueue();
            }
        }

        void cancelAll() {
            const auto queued = std::exchange(m_queue, QList<std::shared_ptr<RequestTask>>{});
            for (const auto &task : queued)
                task->reportCanceledAndFinished();

            const auto ids = m_active.keys();
            for (quint64 id : ids)
                cancelActive(id);
        }

        void shutdown() {
            if (m_shuttingDown)
                return;
            m_shuttingDown = true;
            if (m_cancellationTimer)
                m_cancellationTimer->stop();
            cancelAll();
        }

    private:
        struct Counters {
            int total{};
            QHash<int, int> byCategory;
        };

        struct ActiveTask {
            std::shared_ptr<RequestTask> task;
            QPointer<QNetworkReply> reply;
            QPointer<QTimer> timeoutTimer;
            QPointer<QTimer> retryTimer;
            QDateTime attemptStartedAt;
            bool timedOut{};
        };

        void ensureInitialized() {
            if (m_networkManager)
                return;
            Q_ASSERT(QThread::currentThread() == thread());
            m_networkManager = new QNetworkAccessManager(this);
            m_cancellationTimer = new QTimer(this);
            m_cancellationTimer->setInterval(50);
            connect(m_cancellationTimer, &QTimer::timeout, this, [this] { checkCanceledRequests(); });
            m_cancellationTimer->start();
        }

        bool canStart(const std::shared_ptr<RequestTask> &task) const {
            const auto counters = m_counters.value(task->service.id);
            return counters.total < task->service.globalConcurrency
                && counters.byCategory.value(static_cast<int>(task->category))
                    < task->service.taskConcurrency;
        }

        void pumpQueue() {
            if (m_shuttingDown)
                return;

            for (qsizetype i = 0; i < m_queue.size();) {
                const auto task = m_queue.at(i);
                if (task->isCanceled()) {
                    m_queue.removeAt(i);
                    task->reportCanceledAndFinished();
                    continue;
                }
                if (!canStart(task)) {
                    ++i;
                    continue;
                }
                m_queue.removeAt(i);
                startTask(task);
                // Start the earliest currently admissible request, then rescan because its
                // counters may block a different category while allowing later requests.
                i = 0;
            }
        }

        void startTask(const std::shared_ptr<RequestTask> &task) {
            if (task->isCanceled()) {
                task->reportCanceledAndFinished();
                return;
            }

            task->reportStarted();
            ActiveTask active;
            active.task = task;
            m_active.insert(task->id, active);
            if (task->category != SynthesisCategory::None) {
                auto &counters = m_counters[task->service.id];
                ++counters.total;
                ++counters.byCategory[static_cast<int>(task->category)];
            }
            startAttempt(task->id);
        }

        QUrl requestUrl(const RequestTask &task) const {
            auto url = task.service.baseUrl;
            auto path = url.path();
            while (path.size() > 1 && path.endsWith(u'/'))
                path.chop(1);
            if (path == QStringLiteral("/"))
                path.clear();
            path += apiVersionPrefix(task.apiVersion) + task.route;
            // route contains encoded dynamic path segments. StrictMode preserves those escape
            // sequences while still accepting the normalized endpoint prefix.
            url.setPath(path, QUrl::StrictMode);
            url.setQuery(task.query);
            return url;
        }

        QNetworkRequest networkRequest(const RequestTask &task) const {
            QNetworkRequest request(requestUrl(task));
            request.setRawHeader("Accept", "application/json");
            if (task.method == QByteArrayLiteral("POST"))
                request.setRawHeader("Content-Type", "application/json");
            if (task.service.authenticationEnabled) {
                request.setRawHeader("Authorization",
                                     QByteArrayLiteral("Bearer ") + task.service.apiKey.toUtf8());
            }
            // Do not forward authentication or request bodies to a different URL.
            // DSSP endpoints are expected to be canonical and never redirect.
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                 QNetworkRequest::ManualRedirectPolicy);

            const auto timeoutMilliseconds = std::clamp<qint64>(
                static_cast<qint64>(task.service.requestTimeoutSeconds) * 1000, 1,
                std::numeric_limits<int>::max());
            request.setTransferTimeout(static_cast<int>(timeoutMilliseconds));

#if QT_CONFIG(ssl)
            if (!task.service.verifySslCertificate) {
                auto ssl = request.sslConfiguration();
                ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
                request.setSslConfiguration(ssl);
            }
#endif

            // User-provided headers are deliberately applied last and therefore override all
            // defaults, including Content-Type and Authorization, case-insensitively.
            for (auto it = task.service.customHeaders.cbegin(); it != task.service.customHeaders.cend();
                 ++it) {
                request.setRawHeader(it.key().toLatin1(), it.value().toUtf8());
            }
            return request;
        }

        void startAttempt(quint64 id) {
            auto it = m_active.find(id);
            if (it == m_active.end())
                return;
            auto &active = it.value();
            const auto task = active.task;
            if (task->isCanceled()) {
                cancelActive(id);
                return;
            }

            active.timedOut = false;
            active.attemptStartedAt = QDateTime::currentDateTimeUtc();
            auto request = networkRequest(*task);
            qCDebug(lcDsspApiClient) << "Sending DSSP API request"
                                     << "requestId=" << task->id
                                     << "attempt=" << task->attempt + 1
                                     << "method=" << task->method
                                     << "url=" << request.url().toString(QUrl::FullyEncoded);
            QNetworkReply *reply = nullptr;
            if (task->method == QByteArrayLiteral("GET"))
                reply = m_networkManager->get(request);
            else
                reply = m_networkManager->post(request, task->body);
            active.reply = reply;

#if QT_CONFIG(ssl)
            if (!task->service.verifySslCertificate) {
                connect(reply, &QNetworkReply::sslErrors, reply,
                        [reply](const QList<QSslError> &) { reply->ignoreSslErrors(); });
            }
#endif

            auto *timeout = new QTimer(reply);
            timeout->setSingleShot(true);
            const auto timeoutMilliseconds = std::clamp<qint64>(
                static_cast<qint64>(task->service.requestTimeoutSeconds) * 1000, 1,
                std::numeric_limits<int>::max());
            timeout->setInterval(static_cast<int>(timeoutMilliseconds));
            connect(timeout, &QTimer::timeout, this, [this, id, reply] {
                const auto it = m_active.find(id);
                if (it == m_active.end() || it->reply != reply)
                    return;
                it->timedOut = true;
                reply->abort();
            });
            timeout->start();
            active.timeoutTimer = timeout;

            connect(reply, &QNetworkReply::finished, this, [this, id, reply] {
                finishAttempt(id, reply);
            });
        }

        void finishAttempt(quint64 id, QNetworkReply *reply) {
            auto it = m_active.find(id);
            if (it == m_active.end() || it->reply != reply) {
                reply->deleteLater();
                return;
            }

            auto &active = it.value();
            const auto task = active.task;
            if (active.timeoutTimer)
                active.timeoutTimer->stop();

            const bool timedOut = active.timedOut;
            const auto networkError = timedOut ? QNetworkReply::TimeoutError : reply->error();
            const auto networkErrorString = timedOut ? tr("The request timed out") : reply->errorString();
            const auto statusAttribute = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            const int status = statusAttribute.isValid() ? statusAttribute.toInt() : 0;
            const auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute)
                                    .toString()
                                    .trimmed();
            const auto retryAfter = reply->rawHeader(QByteArrayLiteral("Retry-After"));
            const auto url = reply->url();
            const auto body = reply->readAll();
            const auto rawJson = parseJsonBestEffort(body);
            reply->deleteLater();
            active.reply = nullptr;
            active.timeoutTimer = nullptr;

            if (task->isCanceled()) {
                cancelActive(id);
                return;
            }

            RawResponse response;
            response.httpStatusCode = status;
            response.rawResponse = body;
            response.json = rawJson;

            if (statusAttribute.isValid() && (status < 200 || status >= 300)) {
                ApiError error;
                error.kind = ApiError::ResponseError;
                error.httpStatusCode = status;
                error.networkErrorCode = static_cast<int>(networkError);
                error.message = reason.isEmpty()
                                    ? tr("The synthesis service returned HTTP status %1").arg(status)
                                    : tr("The synthesis service returned HTTP status %1: %2")
                                          .arg(status)
                                          .arg(reason);
                const auto problem = problemDetailsText(rawJson);
                if (!problem.isEmpty()) {
                    error.message += u'\n' + problem;
                } else if (networkError != QNetworkReply::NoError &&
                           !networkErrorString.isEmpty()) {
                    error.message += u'\n' + networkErrorString;
                }
                error.rawResponse = body;
                error.rawJsonResponse = rawJson;
                response.error = std::move(error);
            } else if (statusAttribute.isValid() && networkError != QNetworkReply::NoError) {
                ApiError error;
                error.kind = ApiError::ResponseError;
                error.httpStatusCode = status;
                error.networkErrorCode = static_cast<int>(networkError);
                error.message = tr("The synthesis service returned an incomplete HTTP response: %1")
                                    .arg(networkErrorString);
                error.rawResponse = body;
                error.rawJsonResponse = rawJson;
                response.error = std::move(error);
            } else if (networkError != QNetworkReply::NoError) {
                ApiError error;
                error.kind = ApiError::NetworkError;
                error.networkErrorCode = static_cast<int>(networkError);
                error.httpStatusCode = status;
                error.message = tr("The synthesis service request failed: %1").arg(networkErrorString);
                error.rawResponse = body;
                error.rawJsonResponse = rawJson;
                response.error = std::move(error);
            } else if (!statusAttribute.isValid()) {
                ApiError error;
                error.kind = ApiError::ResponseError;
                error.message = tr("The synthesis service response did not contain an HTTP status code");
                error.rawResponse = body;
                error.rawJsonResponse = rawJson;
                response.error = std::move(error);
            } else {
                QJsonParseError parseError;
                const auto json = QJsonValue::fromJson(body, &parseError);
                if (parseError.error != QJsonParseError::NoError) {
                    ApiError error;
                    error.kind = ApiError::ResponseError;
                    error.httpStatusCode = status;
                    error.message = tr("The synthesis service returned invalid JSON: %1")
                                        .arg(parseError.errorString());
                    error.rawResponse = body;
                    error.rawJsonResponse = rawJson;
                    response.error = std::move(error);
                } else {
                    response.json = json;
                }
            }

            ApiExchange exchange;
            exchange.requestId = task->id;
            exchange.serviceInstanceId = task->service.id;
            exchange.method = task->method;
            exchange.url = url;
            exchange.attempt = task->attempt + 1;
            exchange.httpStatusCode = status;
            exchange.networkErrorCode = static_cast<int>(networkError);
            exchange.startedAt = active.attemptStartedAt;
            exchange.finishedAt = QDateTime::currentDateTimeUtc();
            exchange.requestBody = task->body;
            exchange.responseBody = body;
            if (response.error)
                exchange.errorMessage = response.error->message;
            task->exchanges.append(std::move(exchange));
            response.exchanges = task->exchanges;

            const bool retryable = response.error
                && ((response.error->isNetworkError()
                     && isRetryableNetworkError(static_cast<QNetworkReply::NetworkError>(
                         response.error->networkErrorCode)))
                    || (response.error->isResponseError()
                        && (isRetryableHttpStatus(response.error->httpStatusCode)
                            || (response.error->networkErrorCode != 0
                                && isRetryableNetworkError(
                                    static_cast<QNetworkReply::NetworkError>(
                                        response.error->networkErrorCode))))));
            if (retryable && task->attempt < task->service.retryCount) {
                ++task->attempt;
                const auto delay = retryAfterMilliseconds(retryAfter, task->attempt - 1);
                qCWarning(lcDsspApiClient) << "Retrying DSSP API request"
                                           << "requestId=" << task->id
                                           << "nextAttempt=" << task->attempt + 1
                                           << "delayMs=" << delay
                                           << "error=" << response.error->message;
                auto *timer = new QTimer(this);
                timer->setSingleShot(true);
                timer->setInterval(delay);
                active.retryTimer = timer;
                connect(timer, &QTimer::timeout, this, [this, id, timer] {
                    timer->deleteLater();
                    const auto it = m_active.find(id);
                    if (it == m_active.end())
                        return;
                    it->retryTimer = nullptr;
                    startAttempt(id);
                });
                timer->start();
                return;
            }

            if (response.error) {
                qCWarning(lcDsspApiClient) << "DSSP API request failed"
                                           << "requestId=" << task->id
                                           << "attempts=" << task->attempt + 1
                                           << "httpStatus=" << response.httpStatusCode
                                           << "error=" << response.error->message;
            } else {
                qCDebug(lcDsspApiClient) << "DSSP HTTP request completed"
                                         << "requestId=" << task->id
                                         << "httpStatus=" << response.httpStatusCode
                                         << "responseBytes=" << response.rawResponse.size();
            }

            finishActive(id, response);
        }

        void finishActive(quint64 id, const RawResponse &response) {
            const auto it = m_active.find(id);
            if (it == m_active.end())
                return;
            const auto task = it->task;
            releaseCounters(*task);
            m_active.erase(it);
            task->reportResponse(response);
            pumpQueue();
        }

        void cancelActive(quint64 id) {
            const auto it = m_active.find(id);
            if (it == m_active.end())
                return;
            const auto active = it.value();
            qCDebug(lcDsspApiClient) << "Canceling DSSP API request"
                                     << "requestId=" << active.task->id
                                     << "route=" << active.task->route;
            m_active.erase(it);
            releaseCounters(*active.task);

            if (active.timeoutTimer)
                active.timeoutTimer->stop();
            if (active.retryTimer) {
                active.retryTimer->stop();
                active.retryTimer->deleteLater();
            }
            if (active.reply) {
                QObject::disconnect(active.reply, nullptr, this, nullptr);
                active.reply->abort();
                active.reply->deleteLater();
            }
            active.task->reportCanceledAndFinished();
            pumpQueue();
        }

        void releaseCounters(const RequestTask &task) {
            if (task.category == SynthesisCategory::None)
                return;
            auto it = m_counters.find(task.service.id);
            if (it == m_counters.end())
                return;
            --it->total;
            const int category = static_cast<int>(task.category);
            auto categoryIt = it->byCategory.find(category);
            if (categoryIt != it->byCategory.end() && --categoryIt.value() <= 0)
                it->byCategory.erase(categoryIt);
            if (it->total <= 0)
                m_counters.erase(it);
        }

        void checkCanceledRequests() {
            for (qsizetype i = 0; i < m_queue.size();) {
                if (!m_queue.at(i)->isCanceled()) {
                    ++i;
                    continue;
                }
                const auto task = m_queue.takeAt(i);
                task->reportCanceledAndFinished();
            }

            QList<quint64> canceledIds;
            for (auto it = m_active.cbegin(); it != m_active.cend(); ++it) {
                if (it->task->isCanceled())
                    canceledIds.append(it.key());
            }
            for (quint64 id : canceledIds)
                cancelActive(id);
        }

        QNetworkAccessManager *m_networkManager{};
        QTimer *m_cancellationTimer{};
        QList<std::shared_ptr<RequestTask>> m_queue;
        QHash<quint64, ActiveTask> m_active;
        QHash<QUuid, Counters> m_counters;
        bool m_shuttingDown{};
    };

} // namespace

class ApiClient::Private {
public:
    Private() {
        worker = new NetworkWorker;
        worker->moveToThread(&networkThread);
        QObject::connect(&networkThread, &QThread::finished, worker, &QObject::deleteLater);
        networkThread.setObjectName(QStringLiteral("DSSP API network thread"));
        networkThread.start();
        qCInfo(lcDsspApiClient) << "Started DSSP API network thread";
    }

    ~Private() {
        if (networkThread.isRunning()) {
            qCInfo(lcDsspApiClient) << "Stopping DSSP API network thread";
            QMetaObject::invokeMethod(worker, [this] { worker->shutdown(); },
                                      Qt::BlockingQueuedConnection);
            networkThread.quit();
            networkThread.wait();
            qCInfo(lcDsspApiClient) << "Stopped DSSP API network thread";
        }
    }

    template<typename T>
    QFuture<ApiResult<T>> request(const ServiceInstanceConfiguration &service, const QByteArray &method,
                                  const QString &route, const QUrlQuery &query,
                                  const QByteArray &body, SynthesisCategory category,
                                  ApiVersion apiVersion = ApiVersion::V1) {
        // Pending is a real, waitable Qt future state used by continuations. Until
        // reportStarted() is called by the network scheduler, isStarted()/isRunning() remain
        // false and Qt 6.10 also reports isValid() == false. Callers must therefore use
        // requestState(), not isValid(), to distinguish a queued request.
        auto futureInterface = std::make_shared<QFutureInterface<ApiResult<T>>>(
            QFutureInterfaceBase::Pending);
        auto future = futureInterface->future();

        auto task = std::make_shared<RequestTask>();
        task->id = nextId.fetch_add(1, std::memory_order_relaxed);
        task->service = optionsFrom(service);
        task->method = method;
        task->route = route;
        task->query = query;
        task->body = body;
        task->category = category;
        task->apiVersion = apiVersion;
        task->isCanceled = [futureInterface] { return futureInterface->isCanceled(); };
        task->reportStarted = [futureInterface] {
            if (!futureInterface->isCanceled() && !futureInterface->isStarted())
                futureInterface->reportStarted();
        };
        task->reportCanceledAndFinished = [futureInterface] {
            if (futureInterface->isFinished())
                return;
            if (!futureInterface->isCanceled())
                futureInterface->reportCanceled();
            futureInterface->reportFinished();
        };
        task->reportResponse = [futureInterface](const RawResponse &response) {
            if (futureInterface->isFinished())
                return;
            if (futureInterface->isCanceled()) {
                futureInterface->reportFinished();
                return;
            }

            ApiResult<T> result;
            if (response.error) {
                result = ApiResult<T>::failure(*response.error, response.exchanges);
            } else {
                T value;
                QString parseError;
                if (!T::fromJson(response.json, value, &parseError)) {
                    ApiError error;
                    error.kind = ApiError::ResponseError;
                    error.httpStatusCode = response.httpStatusCode;
                    error.message = Synth::Internal::Api::ApiClient::tr("The synthesis service returned data that does not match the DSSP schema: %1")
                                        .arg(parseError);
                    error.rawResponse = response.rawResponse;
                    error.rawJsonResponse = response.json;
                    auto exchanges = response.exchanges;
                    if (!exchanges.isEmpty())
                        exchanges.last().errorMessage = error.message;
                    result = ApiResult<T>::failure(std::move(error), std::move(exchanges));
                } else {
                    result = ApiResult<T>::success(std::move(value), response.exchanges);
                }
            }
            futureInterface->reportResult(std::move(result));
            futureInterface->reportFinished();
        };

        QMetaObject::invokeMethod(worker, [worker = worker, task] { worker->enqueue(task); },
                                  Qt::QueuedConnection);
        return future;
    }

    void cancelAll() {
        QMetaObject::invokeMethod(worker, [worker = worker] { worker->cancelAll(); },
                                  Qt::QueuedConnection);
    }

    QThread networkThread;
    NetworkWorker *worker{};
    std::atomic<quint64> nextId{1};
};

ApiClient::ApiClient(QObject *parent) : QObject(parent), d(std::make_unique<Private>()) {}

ApiClient::~ApiClient() = default;

QFuture<ApiResult<V1::ApplicationInfoResponse>>
ApiClient::getInfo(const ServiceInstanceConfiguration &service) {
    return d->request<V1::ApplicationInfoResponse>(service, QByteArrayLiteral("GET"),
                                                   QStringLiteral("/info"), {}, {},
                                                   SynthesisCategory::None);
}

QFuture<ApiResult<V1::ArchitectureMetadataList>>
ApiClient::getArchitectures(const ServiceInstanceConfiguration &service,
                            const QString &displayLanguage) {
    QUrlQuery query;
    if (!displayLanguage.isEmpty())
        query.addQueryItem(QStringLiteral("display_language"), displayLanguage);
    return d->request<V1::ArchitectureMetadataList>(service, QByteArrayLiteral("GET"),
                                                    QStringLiteral("/arch"), query, {},
                                                    SynthesisCategory::None);
}

QFuture<ApiResult<V1::ArchitectureMetadata>>
ApiClient::getArchitecture(const ServiceInstanceConfiguration &service,
                           const QString &architectureId, const QString &displayLanguage) {
    QUrlQuery query;
    if (!displayLanguage.isEmpty())
        query.addQueryItem(QStringLiteral("display_language"), displayLanguage);
    return d->request<V1::ArchitectureMetadata>(
        service, QByteArrayLiteral("GET"),
        QStringLiteral("/arch/%1").arg(encodedPathSegment(architectureId)), query, {},
        SynthesisCategory::None);
}

QFuture<ApiResult<V1::SingerInfoList>>
ApiClient::getSingers(const ServiceInstanceConfiguration &service, const QString &displayLanguage) {
    QUrlQuery query;
    if (!displayLanguage.isEmpty())
        query.addQueryItem(QStringLiteral("display_language"), displayLanguage);
    return d->request<V1::SingerInfoList>(service, QByteArrayLiteral("GET"),
                                         QStringLiteral("/singer"), query, {},
                                         SynthesisCategory::None);
}

QFuture<ApiResult<V1::SingerInfoList>>
ApiClient::getArchitectureSingers(const ServiceInstanceConfiguration &service,
                                  const QString &architectureId,
                                  const QString &displayLanguage) {
    QUrlQuery query;
    if (!displayLanguage.isEmpty())
        query.addQueryItem(QStringLiteral("display_language"), displayLanguage);
    return d->request<V1::SingerInfoList>(
        service, QByteArrayLiteral("GET"),
        QStringLiteral("/arch/%1/singer").arg(encodedPathSegment(architectureId)), query, {},
        SynthesisCategory::None);
}

QFuture<ApiResult<V1::SingerInfo>>
ApiClient::getSinger(const ServiceInstanceConfiguration &service, const QString &architectureId,
                     const QString &singerId, const QString &displayLanguage) {
    QUrlQuery query;
    if (!displayLanguage.isEmpty())
        query.addQueryItem(QStringLiteral("display_language"), displayLanguage);
    return d->request<V1::SingerInfo>(
        service, QByteArrayLiteral("GET"),
        QStringLiteral("/arch/%1/singer/%2")
            .arg(encodedPathSegment(architectureId), encodedPathSegment(singerId)),
        query, {}, SynthesisCategory::None);
}

QFuture<ApiResult<V1::SingerAvatarResponse>>
ApiClient::getSingerAvatar(const ServiceInstanceConfiguration &service,
                           const QString &architectureId, const QString &singerId,
                           const QString &displayLanguage) {
    QUrlQuery query;
    if (!displayLanguage.isEmpty())
        query.addQueryItem(QStringLiteral("display_language"), displayLanguage);
    return d->request<V1::SingerAvatarResponse>(
        service, QByteArrayLiteral("GET"),
        QStringLiteral("/arch/%1/singer/%2/avatar")
            .arg(encodedPathSegment(architectureId), encodedPathSegment(singerId)),
        query, {}, SynthesisCategory::None);
}

QFuture<ApiResult<V1::SingerBackgroundResponse>>
ApiClient::getSingerBackground(const ServiceInstanceConfiguration &service,
                               const QString &architectureId, const QString &singerId,
                               const QString &displayLanguage) {
    QUrlQuery query;
    if (!displayLanguage.isEmpty())
        query.addQueryItem(QStringLiteral("display_language"), displayLanguage);
    return d->request<V1::SingerBackgroundResponse>(
        service, QByteArrayLiteral("GET"),
        QStringLiteral("/arch/%1/singer/%2/background")
            .arg(encodedPathSegment(architectureId), encodedPathSegment(singerId)),
        query, {}, SynthesisCategory::None);
}

QFuture<ApiResult<V1::SingerDemoAudioList>>
ApiClient::getSingerDemoAudio(const ServiceInstanceConfiguration &service,
                              const QString &architectureId, const QString &singerId,
                              const QString &displayLanguage) {
    QUrlQuery query;
    if (!displayLanguage.isEmpty())
        query.addQueryItem(QStringLiteral("display_language"), displayLanguage);
    return d->request<V1::SingerDemoAudioList>(
        service, QByteArrayLiteral("GET"),
        QStringLiteral("/arch/%1/singer/%2/demo_audio")
            .arg(encodedPathSegment(architectureId), encodedPathSegment(singerId)),
        query, {}, SynthesisCategory::None);
}

QFuture<ApiResult<V1::EnvTagResponse>>
ApiClient::createEnvironmentTag(const ServiceInstanceConfiguration &service,
                                const V1::EnvTagRequest &request) {
    return d->request<V1::EnvTagResponse>(service, QByteArrayLiteral("POST"),
                                         QStringLiteral("/env_tag"), {}, jsonBody(request.toJson()),
                                         SynthesisCategory::None);
}

QFuture<ApiResult<V1::PronunciationResponse>>
ApiClient::synthesizePronunciation(const ServiceInstanceConfiguration &service,
                                   const V1::PronunciationRequest &request) {
    return d->request<V1::PronunciationResponse>(
        service, QByteArrayLiteral("POST"), QStringLiteral("/synth/pronunciation"), {},
        jsonBody(request.toJson()), SynthesisCategory::Pronunciation);
}

QFuture<ApiResult<V1::PhonemeResponse>>
ApiClient::synthesizePhoneme(const ServiceInstanceConfiguration &service,
                             const V1::PhonemeRequest &request) {
    return d->request<V1::PhonemeResponse>(service, QByteArrayLiteral("POST"),
                                          QStringLiteral("/synth/phoneme"), {},
                                          jsonBody(request.toJson()), SynthesisCategory::Phoneme);
}

QFuture<ApiResult<V1::DurationResponse>>
ApiClient::synthesizeDuration(const ServiceInstanceConfiguration &service,
                              const V1::DurationRequest &request) {
    return d->request<V1::DurationResponse>(service, QByteArrayLiteral("POST"),
                                           QStringLiteral("/synth/duration"), {},
                                           jsonBody(request.toJson()), SynthesisCategory::Duration);
}

QFuture<ApiResult<V1::ParameterResponse>>
ApiClient::synthesizeParameter(const ServiceInstanceConfiguration &service,
                               const V1::ParameterRequest &request) {
    return d->request<V1::ParameterResponse>(service, QByteArrayLiteral("POST"),
                                            QStringLiteral("/synth/parameter"), {},
                                            jsonBody(request.toJson()), SynthesisCategory::Parameter);
}

QFuture<ApiResult<V1::AudioResponse>>
ApiClient::synthesizeAudio(const ServiceInstanceConfiguration &service,
                           const V1::AudioRequest &request) {
    return d->request<V1::AudioResponse>(service, QByteArrayLiteral("POST"),
                                        QStringLiteral("/synth/audio"), {},
                                        jsonBody(request.toJson()), SynthesisCategory::Audio);
}

void ApiClient::cancelAll() { d->cancelAll(); }

void ApiClient::shutdown() { d.reset(); }

} // namespace Synth::Internal::Api

#include "ApiClient.moc"
