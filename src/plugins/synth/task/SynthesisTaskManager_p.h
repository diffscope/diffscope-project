#ifndef DIFFSCOPE_SYNTH_SYNTHESISTASKMANAGER_P_H
#define DIFFSCOPE_SYNTH_SYNTHESISTASKMANAGER_P_H

#include <functional>
#include <optional>
#include <utility>

#include <QDateTime>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QSet>

#include <synth/ServiceTypes.h>
#include <synth/SynthesisTask.h>
#include <synth/SynthesisTaskManager.h>
#include <synth/internal/ApiClient.h>
#include <synth/internal/SynthesisTaskCache.h>

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

namespace Synth {

    class SynthesisTaskManagerPrivate {
        Q_DECLARE_PUBLIC(SynthesisTaskManager)

    public:
        enum class ServiceAvailability {
            Ready,
            Waiting,
            Unavailable,
        };

        struct ServiceResolution {
            ServiceAvailability availability{ServiceAvailability::Unavailable};
            std::optional<ServiceInstanceConfiguration> service;
        };

        struct EnvironmentTagEntry {
            QString tag;
            QDateTime expiresAt;
        };

        struct PendingEnvironmentTag {
            QList<QPointer<SynthesisTask>> tasks;
        };

        explicit SynthesisTaskManagerPrivate(SynthesisTaskManager *q);

        static QString counterKey(const QUuid &serviceId, SynthesisTaskType type);

        ServiceResolution resolveService(const SynthesisContext &context) const;
        static bool providesContext(const ServiceInstanceDetails &details, const SynthesisContext &context);
        ArchitectureMetadata architectureMetadata(const ServiceInstanceConfiguration &service, const QString &architectureId) const;
        QJsonObject metadataJson(const ServiceInstanceConfiguration &service, const SynthesisContext &context) const;

        QJsonObject cacheEnvelope(const ServiceInstanceConfiguration &service, const SynthesisTaskRequest &request, const QString &environmentTag) const;
        QByteArray taskCacheKey(const ServiceInstanceConfiguration &service, const SynthesisTaskRequest &request, const QString &environmentTag) const;
        QByteArray parameterCacheKey(const ServiceInstanceConfiguration &service, const SynthesisTaskRequest &request, const QString &environmentTag, const QString &target, const QMap<QString, SynthesisParameter> &parameters, const QSet<QString> &unavailableParameters = {}) const;

        void setService(SynthesisTask *task, const ServiceInstanceConfiguration &service);
        void setState(SynthesisTask *task, SynthesisTask::State state, const QString &error = {});
        bool canStart(const ServiceInstanceConfiguration &service, SynthesisTaskType type) const;
        void pump();
        void start(SynthesisTask *task, const ServiceInstanceConfiguration &service);
        void releaseSlot(SynthesisTask *task);
        void complete(SynthesisTask *task, SynthesisTaskResult result);
        void fail(SynthesisTask *task, const QString &message);
        void finishCanceled(SynthesisTask *task);
        void requeue(SynthesisTask *task, const std::optional<ServiceInstanceConfiguration> &service = std::nullopt);
        void appendExchanges(SynthesisTask *task, const QList<Internal::Api::ApiExchange> &exchanges);
        void appendExchange(SynthesisTask *task, const Internal::Api::ApiExchange &exchange);
        void publishDiagnostics(SynthesisTask *task);
        QJsonObject diagnosticDocument(SynthesisTask *task, const QString &message) const;
        void persistDiagnostics(SynthesisTask *task, const QString &message);
        void trimDiagnostics();

        template <typename T, typename Callback>
        void watch(SynthesisTask *task, QFuture<Internal::Api::ApiResult<T>> future, Callback callback) {
            auto watcher = new QFutureWatcher<Internal::Api::ApiResult<T>>(q_ptr);
            cancelFunctions.insert(task, [future]() mutable { future.cancel(); });
            QObject::connect(watcher, &QFutureWatcherBase::finished, q_ptr, [this, watcher, task, callback = std::move(callback)]() mutable {
                cancelFunctions.remove(task);
                if (watcher->future().isCanceled()) {
                    watcher->deleteLater();
                    finishCanceled(task);
                    return;
                }
                const auto result = watcher->result();
                watcher->deleteLater();
                appendExchanges(task, result.exchanges());
                callback(std::move(result));
            });
            watcher->setFuture(future);
        }

        QByteArray environmentKey(const ServiceInstanceConfiguration &service, const SynthesisContext &context) const;
        void obtainEnvironmentTag(SynthesisTask *task, const ServiceInstanceConfiguration &service);
        void requestEnvironmentTag(const QByteArray &key, const ServiceInstanceConfiguration &service, const SynthesisContext &context);
        void execute(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag);
        void maybeWriteAndComplete(SynthesisTask *task, const QByteArray &key, SynthesisTaskResult result);
        void executePronunciation(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag, const QByteArray &key);
        void executePhoneme(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag, const QByteArray &key);
        void executeDuration(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag, const QByteArray &key);
        void executeParameter(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag);
        void executeAudio(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag, const QByteArray &key);

        QString suffixForMime(const QString &mimeName) const;
        void finishAudioBytes(SynthesisTask *task, const QByteArray &key, const QByteArray &bytes, const QString &suffix);
        void materializeAudio(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QByteArray &key, const QString &location);
        static int effectivePort(const QUrl &url);
        static bool sameOrigin(const QUrl &left, const QUrl &right);
        void downloadAudio(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QByteArray &key, const QUrl &url, int redirectCount);

        SynthesisTaskManager *q_ptr{};
        Internal::Api::ApiClient *apiClient{};
        QNetworkAccessManager *networkManager{};
        Internal::SynthesisTaskCache cache;
        QList<SynthesisTask *> tasks;
        QList<SynthesisTask *> queue;
        QHash<QUuid, int> activeByService;
        QHash<QString, int> activeByServiceAndType;
        QHash<SynthesisTask *, std::function<void()>> cancelFunctions;
        QHash<QByteArray, EnvironmentTagEntry> environmentTags;
        QHash<QByteArray, PendingEnvironmentTag> pendingEnvironmentTags;
        QHash<SynthesisTask *, QList<Internal::Api::ApiExchange>> diagnosticExchanges;
        QString diagnosticsRoot;
        bool shuttingDown{};
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISTASKMANAGER_P_H
