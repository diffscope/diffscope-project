#ifndef DIFFSCOPE_SYNTH_INTERNAL_APITYPES_H
#define DIFFSCOPE_SYNTH_INTERNAL_APITYPES_H

#include <optional>
#include <utility>

#include <QByteArray>
#include <QFuture>
#include <QJsonValue>
#include <QMetaType>
#include <QString>

namespace Synth::Internal::Api {

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
        static ApiResult success(T value) {
            ApiResult result;
            result.m_value = std::move(value);
            return result;
        }

        static ApiResult failure(ApiError error) {
            ApiResult result;
            result.m_error = std::move(error);
            return result;
        }

        bool hasValue() const { return m_value.has_value(); }
        bool hasError() const { return m_error.has_value(); }
        explicit operator bool() const { return hasValue(); }

        const T &value() const { return m_value.value(); }
        T &value() { return m_value.value(); }
        T takeValue() { return std::move(m_value).value(); }

        const ApiError &error() const { return m_error.value(); }

    private:
        std::optional<T> m_value;
        std::optional<ApiError> m_error;
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

#endif // DIFFSCOPE_SYNTH_INTERNAL_APITYPES_H
