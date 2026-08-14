#ifndef DIFFSCOPE_SYNTH_INTERNAL_APITYPES_H
#define DIFFSCOPE_SYNTH_INTERNAL_APITYPES_H

#include <optional>
#include <utility>

#include <QByteArray>
#include <QDateTime>
#include <QFuture>
#include <QJsonValue>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QUrl>
#include <QUuid>

namespace Synth::Internal::Api {

    struct ApiExchange {
        quint64 requestId{};
        QUuid serviceInstanceId;
        QByteArray method;
        QUrl url;
        int attempt{};
        int httpStatusCode{};
        int networkErrorCode{};
        QDateTime startedAt;
        QDateTime finishedAt;
        QByteArray requestBody;
        QByteArray responseBody;
        QString errorMessage;
    };

    struct ApiError {
        Q_GADGET
    public:
        enum Kind {
            NetworkError,
            ResponseError,
        };
        Q_ENUM(Kind)

        Kind kind{NetworkError};
        int networkErrorCode{};
        int httpStatusCode{};
        QString message;
        QByteArray rawResponse;
        QJsonValue rawJsonResponse{QJsonValue::Undefined};

        bool isNetworkError() const { return kind == NetworkError; }
        bool isResponseError() const { return kind == ResponseError; }
    };

    template<typename T>
    class ApiResult {
    public:
        static ApiResult success(T value, QList<ApiExchange> exchanges = {}) {
            ApiResult result;
            result.m_value = std::move(value);
            result.m_exchanges = std::move(exchanges);
            return result;
        }

        static ApiResult failure(ApiError error, QList<ApiExchange> exchanges = {}) {
            ApiResult result;
            result.m_error = std::move(error);
            result.m_exchanges = std::move(exchanges);
            return result;
        }

        bool hasValue() const { return m_value.has_value(); }
        bool hasError() const { return m_error.has_value(); }
        explicit operator bool() const { return hasValue(); }

        const T &value() const { return m_value.value(); }
        T &value() { return m_value.value(); }
        T takeValue() { return std::move(m_value).value(); }

        const ApiError &error() const { return m_error.value(); }
        const QList<ApiExchange> &exchanges() const { return m_exchanges; }

    private:
        std::optional<T> m_value;
        std::optional<ApiError> m_error;
        QList<ApiExchange> m_exchanges;
    };

    enum class AsyncRequestState {
        Waiting,
        Running,
        Finished,
        Canceled,
    };

    template<typename T>
    AsyncRequestState requestState(const QFuture<ApiResult<T>> &future) {
        if (future.isCanceled())
            return AsyncRequestState::Canceled;
        if (future.isFinished())
            return AsyncRequestState::Finished;
        if (future.isStarted() || future.isRunning())
            return AsyncRequestState::Running;
        return AsyncRequestState::Waiting;
    }

} // namespace Synth::Internal::Api

Q_DECLARE_METATYPE(Synth::Internal::Api::ApiError)
Q_DECLARE_METATYPE(Synth::Internal::Api::ApiExchange)

#endif // DIFFSCOPE_SYNTH_INTERNAL_APITYPES_H
