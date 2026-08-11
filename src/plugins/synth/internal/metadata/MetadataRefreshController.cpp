#include "MetadataRefreshController.h"

#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QHash>
#include <QLocale>
#include <QLoggingCategory>
#include <QPointer>
#include <QSet>
#include <QTimer>

#include <synth/internal/ApiClient.h>
#include <synth/internal/ApiVersion.h>
#include <synth/internal/MetadataConverter.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcMetadataRefreshController, "diffscope.synth.metadatarefresh")

    namespace {

        QString apiErrorText(const Api::ApiError &error) {
            if (!error.message.isEmpty())
                return error.message;
            if (error.isResponseError())
                return MetadataRefreshController::tr("HTTP response error %1").arg(error.httpStatusCode);
            return MetadataRefreshController::tr("Network error %1").arg(error.networkErrorCode);
        }

        QString singerKey(const QString &architectureId, const QString &singerId) {
            return architectureId + QChar::Null + singerId;
        }

        const char *healthStatusName(ServiceInstanceDetails::HealthStatus status) {
            switch (status) {
                case ServiceInstanceDetails::Disabled: return "disabled";
                case ServiceInstanceDetails::Unknown: return "unknown";
                case ServiceInstanceDetails::Checking: return "checking";
                case ServiceInstanceDetails::Healthy: return "healthy";
                case ServiceInstanceDetails::Error: return "error";
            }
            return "invalid";
        }

    }

    class MetadataRefreshController::Private {
    public:
        struct State {
            ServiceInstanceConfiguration configuration;
            ServiceInstanceDetails details;
            QPointer<QTimer> timer;
            bool removed{};
            bool active{};
            bool activeIncludesMetadata{};
            bool pending{};
            bool pendingIncludesMetadata{};
            quint64 generation{};
            quint64 activeGeneration{};
            QHash<quint64, QList<QPointer<QFutureWatcherBase>>> watchers;
            QElapsedTimer elapsed;

            bool architecturesDone{};
            bool singersDone{};
            std::optional<Api::ApiResult<Api::V1::ArchitectureMetadataList>> architecturesResult;
            std::optional<Api::ApiResult<Api::V1::SingerInfoList>> singersResult;
            QList<ArchitectureMetadata> refreshedArchitectures;
            QList<SingerMetadata> refreshedSingers;
            int pendingAssets{};
            QStringList assetErrors;
            QString healthError;
            QString metadataError;
        };

        explicit Private(MetadataRefreshController *q, Api::ApiClient *client)
            : q(q), apiClient(client) {
        }

        MetadataRefreshController *q;
        Api::ApiClient *apiClient{};
        QHash<QUuid, std::shared_ptr<State>> states;
        QList<QUuid> order;
        bool started{};
        bool stopping{};
        int activeCount{};

        template<typename T, typename Handler>
        void watch(const std::shared_ptr<State> &state, quint64 generation,
                   QFuture<Api::ApiResult<T>> future, Handler &&handler) {
            auto watcher = new QFutureWatcher<Api::ApiResult<T>>(q);
            state->watchers[generation].append(watcher);
            QObject::connect(watcher, &QFutureWatcherBase::finished, q,
                             [this, state, generation, watcher,
                              handler = std::forward<Handler>(handler)]() mutable {
                auto iterator = state->watchers.find(generation);
                if (iterator != state->watchers.end()) {
                    iterator->removeAll(watcher);
                    if (iterator->isEmpty())
                        state->watchers.erase(iterator);
                }
                const auto future = watcher->future();
                if (!future.isCanceled() && future.resultCount() > 0)
                    handler(std::optional<Api::ApiResult<T>>(future.result()));
                else
                    handler(std::optional<Api::ApiResult<T>>{});

                // A service edit invalidates the current generation, but it deliberately
                // remains logically active until all of that generation's futures have
                // observed cancellation. This prevents a request using the new endpoint
                // from overlapping an old request (and its old credentials).
                if (!stopping && !state->removed && state->active &&
                    state->activeGeneration == generation && generation != state->generation &&
                    !state->watchers.contains(generation)) {
                    finish(state);
                }
                watcher->deleteLater();
            });
            watcher->setFuture(future);
        }

        void cancelActiveRequests(const std::shared_ptr<State> &state) {
            const auto iterator = state->watchers.constFind(state->activeGeneration);
            if (iterator == state->watchers.cend())
                return;
            for (const auto &watcher : *iterator) {
                if (watcher)
                    watcher->cancel();
            }
        }

        void emitDetails(const std::shared_ptr<State> &state, bool metadataChanged = false) {
            if (state->removed)
                return;
            Q_EMIT q->serviceDetailsChanged(state->configuration.id());
            if (metadataChanged)
                Q_EMIT q->metadataChanged();
        }

        void syncError(const std::shared_ptr<State> &state) {
            QStringList errors;
            if (!state->healthError.isEmpty())
                errors.append(state->healthError);
            if (!state->metadataError.isEmpty() && state->metadataError != state->healthError)
                errors.append(state->metadataError);
            state->details.setErrorMessage(errors.join(QStringLiteral("; ")));
        }

        bool isCurrent(const std::shared_ptr<State> &state, quint64 generation) const {
            return !stopping && !state->removed && state->active &&
                state->activeGeneration == generation && state->generation == generation;
        }

        void setActive(const std::shared_ptr<State> &state, bool active) {
            if (state->active == active)
                return;
            const bool wasRefreshing = activeCount > 0;
            state->active = active;
            activeCount += active ? 1 : -1;
            activeCount = qMax(0, activeCount);
            if (wasRefreshing != (activeCount > 0))
                Q_EMIT q->refreshingChanged();
        }

        void scheduleTimer(const std::shared_ptr<State> &state) {
            if (!started || stopping || state->removed || state->active ||
                !state->configuration.isEnabled() || !state->timer)
                return;
            const auto interval = std::clamp<qint64>(
                static_cast<qint64>(state->configuration.healthCheckIntervalSeconds()) * 1000,
                1, std::numeric_limits<int>::max());
            state->timer->setInterval(static_cast<int>(interval));
            state->timer->start();
        }

        void finish(const std::shared_ptr<State> &state) {
            if (!state->active)
                return;

            const bool restart = state->pending;
            const bool restartIncludesMetadata = state->pendingIncludesMetadata;
            const bool includedMetadata = state->activeIncludesMetadata;
            const auto elapsedMilliseconds = state->elapsed.isValid()
                ? state->elapsed.elapsed()
                : -1;
            state->pending = false;
            state->pendingIncludesMetadata = false;
            state->activeIncludesMetadata = false;
            state->activeGeneration = 0;
            setActive(state, false);

            if (includedMetadata) {
                qCInfo(lcMetadataRefreshController)
                    << "Finished DSSP metadata refresh"
                    << "service=" << state->configuration.name()
                    << "status=" << healthStatusName(state->details.healthStatus())
                    << "elapsedMs=" << elapsedMilliseconds;
            } else {
                qCDebug(lcMetadataRefreshController)
                    << "Finished DSSP health check"
                    << "service=" << state->configuration.name()
                    << "status=" << healthStatusName(state->details.healthStatus())
                    << "elapsedMs=" << elapsedMilliseconds;
            }

            if (stopping || state->removed)
                return;

            // refreshingChanged is synchronous. A slot may already have started a replacement
            // request while setActive(false) was emitting it; never overwrite that request's
            // generation, and only retain work it does not already cover.
            if (state->active) {
                if (restart && restartIncludesMetadata && !state->activeIncludesMetadata) {
                    state->pending = true;
                    state->pendingIncludesMetadata = true;
                }
                return;
            }

            scheduleTimer(state);
            if (restart) {
                QTimer::singleShot(0, q, [this, state, restartIncludesMetadata] {
                    request(state, restartIncludesMetadata);
                });
            }
        }

        bool failMetadata(const std::shared_ptr<State> &state, quint64 generation,
                          const QString &message) {
            if (!isCurrent(state, generation))
                return false;
            state->details.setMetadataStale(
                state->details.lastMetadataRefresh().isValid());
            state->metadataError = message;
            qCWarning(lcMetadataRefreshController)
                << "DSSP metadata refresh failed"
                << "service=" << state->configuration.name()
                << "error=" << message;
            syncError(state);
            emitDetails(state);
            if (!isCurrent(state, generation))
                return false;
            Q_EMIT q->metadataRefreshFailed(state->configuration.name(), message);
            return isCurrent(state, generation);
        }

        void request(const std::shared_ptr<State> &state, bool includeMetadata) {
            if (stopping || state->removed) {
                qCDebug(lcMetadataRefreshController)
                    << "Ignoring refresh for a stopped or removed DSSP service";
                return;
            }
            if (!state->configuration.isEnabled()) {
                qCDebug(lcMetadataRefreshController)
                    << "Skipping disabled DSSP service"
                    << "service=" << state->configuration.name();
                state->details.setHealthStatus(ServiceInstanceDetails::Disabled);
                state->details.setConfiguration(state->configuration);
                emitDetails(state);
                return;
            }
            if (state->active) {
                qCDebug(lcMetadataRefreshController)
                    << "Queueing refresh behind an active DSSP request"
                    << "service=" << state->configuration.name()
                    << "includeMetadata=" << includeMetadata;
                state->pending = true;
                state->pendingIncludesMetadata |= includeMetadata;
                return;
            }

            if (state->timer)
                state->timer->stop();
            const auto generation = ++state->generation;
            state->activeIncludesMetadata = includeMetadata;
            state->activeGeneration = generation;
            state->elapsed.start();
            if (includeMetadata) {
                qCInfo(lcMetadataRefreshController)
                    << "Starting DSSP health and metadata refresh"
                    << "service=" << state->configuration.name()
                    << "url=" << state->configuration.baseUrl().toString(QUrl::FullyEncoded)
                    << "generation=" << generation;
            } else {
                qCDebug(lcMetadataRefreshController)
                    << "Starting DSSP health check"
                    << "service=" << state->configuration.name()
                    << "url=" << state->configuration.baseUrl().toString(QUrl::FullyEncoded)
                    << "generation=" << generation;
            }
            const auto previousHealth = state->details.healthStatus();
            state->details.setConfiguration(state->configuration);
            state->details.setHealthStatus(ServiceInstanceDetails::Checking);
            state->healthError.clear();
            syncError(state);
            setActive(state, true);
            if (!isCurrent(state, generation)) {
                if (state->active && state->activeGeneration == generation)
                    finish(state);
                return;
            }
            emitDetails(state);
            if (!isCurrent(state, generation)) {
                if (state->active && state->activeGeneration == generation)
                    finish(state);
                return;
            }

            watch(state, generation, apiClient->getInfo(state->configuration),
                  [this, state, generation, includeMetadata, previousHealth](
                      std::optional<Api::ApiResult<Api::V1::ApplicationInfoResponse>> result) {
                if (!isCurrent(state, generation))
                    return;
                state->details.setLastHealthCheck(QDateTime::currentDateTimeUtc());
                const auto negotiatedVersion = result && result->hasValue()
                    ? Api::negotiateApiVersion(result->value().dssp.apiVersion)
                    : std::nullopt;
                if (!result || result->hasError() || !negotiatedVersion) {
                    const QString message = !result
                        ? MetadataRefreshController::tr("The health check was canceled.")
                        : result->hasError()
                            ? apiErrorText(result->error())
                            : MetadataRefreshController::tr("The service does not support DSSP API version 1.");
                    state->details.setHealthStatus(ServiceInstanceDetails::Error);
                    state->details.setMaximumApiVersion(0);
                    state->details.setSelectedApiVersion(0);
                    state->healthError = message;
                    qCWarning(lcMetadataRefreshController)
                        << "DSSP health check failed"
                        << "service=" << state->configuration.name()
                        << "error=" << message;
                    syncError(state);
                    emitDetails(state);
                    if (!isCurrent(state, generation))
                        return;
                    if (previousHealth == ServiceInstanceDetails::Healthy)
                        Q_EMIT q->serviceBecameUnhealthy(state->configuration.name(), message);
                    if (!isCurrent(state, generation))
                        return;
                    if (includeMetadata && !failMetadata(state, generation, message))
                        return;
                    finish(state);
                    return;
                }

                const int maximumVersion = result->value().dssp.apiVersion;
                if (includeMetadata) {
                    qCInfo(lcMetadataRefreshController)
                        << "DSSP health check succeeded"
                        << "service=" << state->configuration.name()
                        << "maximumApiVersion=" << maximumVersion
                        << "selectedApiVersion=" << static_cast<int>(*negotiatedVersion);
                } else {
                    qCDebug(lcMetadataRefreshController)
                        << "DSSP health check succeeded"
                        << "service=" << state->configuration.name()
                        << "maximumApiVersion=" << maximumVersion
                        << "selectedApiVersion=" << static_cast<int>(*negotiatedVersion);
                }
                state->details.setHealthStatus(ServiceInstanceDetails::Healthy);
                state->details.setMaximumApiVersion(maximumVersion);
                state->details.setSelectedApiVersion(static_cast<int>(*negotiatedVersion));
                state->healthError.clear();
                syncError(state);
                emitDetails(state);
                if (!isCurrent(state, generation))
                    return;
                if (!includeMetadata) {
                    finish(state);
                    return;
                }
                beginMetadata(state, generation);
            });
        }

        void beginMetadata(const std::shared_ptr<State> &state, quint64 generation) {
            state->architecturesDone = false;
            state->singersDone = false;
            state->architecturesResult.reset();
            state->singersResult.reset();
            const auto displayLanguage = QLocale().bcp47Name();
            qCDebug(lcMetadataRefreshController)
                << "Requesting DSSP architecture and singer metadata"
                << "service=" << state->configuration.name()
                << "displayLanguage=" << displayLanguage;

            watch(state, generation,
                  apiClient->getArchitectures(state->configuration, displayLanguage),
                  [this, state, generation](
                      std::optional<Api::ApiResult<Api::V1::ArchitectureMetadataList>> result) {
                if (!isCurrent(state, generation))
                    return;
                state->architecturesDone = true;
                state->architecturesResult = std::move(result);
                finishMetadataLists(state, generation);
            });
            watch(state, generation, apiClient->getSingers(state->configuration, displayLanguage),
                  [this, state, generation](
                      std::optional<Api::ApiResult<Api::V1::SingerInfoList>> result) {
                if (!isCurrent(state, generation))
                    return;
                state->singersDone = true;
                state->singersResult = std::move(result);
                finishMetadataLists(state, generation);
            });
        }

        void finishMetadataLists(const std::shared_ptr<State> &state, quint64 generation) {
            if (!isCurrent(state, generation) || !state->architecturesDone ||
                !state->singersDone)
                return;
            QStringList errors;
            if (!state->architecturesResult || state->architecturesResult->hasError()) {
                errors.append(state->architecturesResult
                                  ? apiErrorText(state->architecturesResult->error())
                                  : MetadataRefreshController::tr("The architecture request was canceled."));
            }
            if (!state->singersResult || state->singersResult->hasError()) {
                errors.append(state->singersResult
                                  ? apiErrorText(state->singersResult->error())
                                  : MetadataRefreshController::tr("The singer request was canceled."));
            }
            if (!errors.isEmpty()) {
                if (!failMetadata(state, generation, errors.join(QStringLiteral("; "))))
                    return;
                finish(state);
                return;
            }

            qCDebug(lcMetadataRefreshController)
                << "Received DSSP metadata lists"
                << "service=" << state->configuration.name()
                << "architectures=" << state->architecturesResult->value().items.size()
                << "singers=" << state->singersResult->value().items.size();

            state->refreshedArchitectures.clear();
            for (const auto &item : state->architecturesResult->value().items)
                state->refreshedArchitectures.append(MetadataConverter::architecture(item));

            QHash<QString, SingerMetadata> previousSingers;
            for (const auto &item : state->details.metadata().singers())
                previousSingers.insert(singerKey(item.architectureId(), item.id()), item);

            state->refreshedSingers.clear();
            for (const auto &item : state->singersResult->value().items) {
                auto converted = MetadataConverter::singer(item, state->configuration.id());
                const auto previous = previousSingers.constFind(singerKey(item.arch, item.id));
                if (previous != previousSingers.cend()) {
                    converted.setAvatarUrl(previous->avatarUrl());
                    converted.setBackgroundUrl(previous->backgroundUrl());
                    converted.setDemos(previous->demos());
                }
                state->refreshedSingers.append(converted);
            }
            state->assetErrors.clear();
            state->pendingAssets = state->refreshedSingers.size() * 3;
            if (state->pendingAssets == 0) {
                commitMetadata(state, generation);
                return;
            }

            const auto displayLanguage = QLocale().bcp47Name();
            for (qsizetype index = 0; index < state->refreshedSingers.size(); ++index) {
                const auto architectureId = state->refreshedSingers.at(index).architectureId();
                const auto singerId = state->refreshedSingers.at(index).id();
                watch(state, generation,
                      apiClient->getSingerAvatar(state->configuration, architectureId, singerId,
                                                 displayLanguage),
                      [this, state, generation, index](
                          std::optional<Api::ApiResult<Api::V1::SingerAvatarResponse>> result) {
                    if (!stopping && !state->removed && generation == state->generation && result &&
                        result->hasValue()) {
                        auto singer = state->refreshedSingers.at(index);
                        singer.setAvatarUrl(QUrl(result->value().avatarUrl));
                        state->refreshedSingers[index] = singer;
                    } else if (!stopping && generation == state->generation) {
                        state->assetErrors.append(MetadataRefreshController::tr("Could not refresh a singer avatar."));
                    }
                    finishAsset(state, generation);
                });
                watch(state, generation,
                      apiClient->getSingerBackground(state->configuration, architectureId, singerId,
                                                     displayLanguage),
                      [this, state, generation, index](
                          std::optional<Api::ApiResult<Api::V1::SingerBackgroundResponse>> result) {
                    if (!stopping && !state->removed && generation == state->generation && result &&
                        result->hasValue()) {
                        auto singer = state->refreshedSingers.at(index);
                        singer.setBackgroundUrl(QUrl(result->value().backgroundUrl));
                        state->refreshedSingers[index] = singer;
                    } else if (!stopping && generation == state->generation) {
                        state->assetErrors.append(MetadataRefreshController::tr("Could not refresh a singer background."));
                    }
                    finishAsset(state, generation);
                });
                watch(state, generation,
                      apiClient->getSingerDemoAudio(state->configuration, architectureId, singerId,
                                                    displayLanguage),
                      [this, state, generation, index](
                          std::optional<Api::ApiResult<Api::V1::SingerDemoAudioList>> result) {
                    if (!stopping && !state->removed && generation == state->generation && result &&
                        result->hasValue()) {
                        auto singer = state->refreshedSingers.at(index);
                        singer.setDemos(MetadataConverter::demos(result->value()));
                        state->refreshedSingers[index] = singer;
                    } else if (!stopping && generation == state->generation) {
                        state->assetErrors.append(MetadataRefreshController::tr("Could not refresh singer demo audio."));
                    }
                    finishAsset(state, generation);
                });
            }
        }

        void finishAsset(const std::shared_ptr<State> &state, quint64 generation) {
            if (stopping || state->removed || generation != state->generation || !state->active)
                return;
            if (--state->pendingAssets == 0)
                commitMetadata(state, generation);
        }

        void commitMetadata(const std::shared_ptr<State> &state, quint64 generation) {
            if (!isCurrent(state, generation))
                return;
            state->assetErrors.removeDuplicates();
            ServiceMetadata metadata;
            metadata.setArchitectures(state->refreshedArchitectures);
            metadata.setSingers(state->refreshedSingers);
            state->details.setMetadata(metadata);
            state->details.setLastMetadataRefresh(QDateTime::currentDateTimeUtc());
            state->details.setMetadataStale(!state->assetErrors.isEmpty());
            state->metadataError = state->assetErrors.join(QStringLiteral("; "));
            qCInfo(lcMetadataRefreshController)
                << "Committed DSSP metadata cache"
                << "service=" << state->configuration.name()
                << "architectures=" << state->refreshedArchitectures.size()
                << "singers=" << state->refreshedSingers.size()
                << "assetErrors=" << state->assetErrors.size();
            if (!state->assetErrors.isEmpty()) {
                qCWarning(lcMetadataRefreshController)
                    << "DSSP singer media refresh completed with errors"
                    << "service=" << state->configuration.name()
                    << "errors=" << state->metadataError;
            }
            syncError(state);
            emitDetails(state, true);
            if (!isCurrent(state, generation))
                return;
            if (!state->assetErrors.isEmpty()) {
                Q_EMIT q->metadataRefreshFailed(state->configuration.name(), state->details.errorMessage());
                if (!isCurrent(state, generation))
                    return;
            }
            finish(state);
        }
    };

    MetadataRefreshController::MetadataRefreshController(Api::ApiClient *apiClient, QObject *parent)
        : QObject(parent), d(std::make_unique<Private>(this, apiClient)) {
        Q_ASSERT(apiClient);
    }

    MetadataRefreshController::~MetadataRefreshController() {
        stop();
    }

    void MetadataRefreshController::setServices(const QList<ServiceInstanceConfiguration> &services) {
        qCInfo(lcMetadataRefreshController) << "Updating DSSP refresh controller with"
                                            << services.size() << "service instance(s)";
        QSet<QUuid> currentIds;
        QList<QUuid> newOrder;
        newOrder.reserve(services.size());
        for (const auto &configuration : services) {
            currentIds.insert(configuration.id());
            newOrder.append(configuration.id());
            auto state = d->states.value(configuration.id());
            if (!state) {
                qCDebug(lcMetadataRefreshController)
                    << "Adding DSSP service to refresh controller"
                    << "service=" << configuration.name()
                    << "enabled=" << configuration.isEnabled()
                    << "url=" << configuration.baseUrl().toString(QUrl::FullyEncoded);
                state = std::make_shared<Private::State>();
                state->configuration = configuration;
                state->details.setConfiguration(configuration);
                state->details.setHealthStatus(configuration.isEnabled()
                                                   ? ServiceInstanceDetails::Unknown
                                                   : ServiceInstanceDetails::Disabled);
                auto timer = new QTimer(this);
                timer->setSingleShot(true);
                state->timer = timer;
                connect(timer, &QTimer::timeout, this, [this, state] {
                    qCDebug(lcMetadataRefreshController)
                        << "Periodic DSSP health check triggered"
                        << "service=" << state->configuration.name();
                    d->request(state, false);
                });
                d->states.insert(configuration.id(), state);
            } else {
                const auto old = state->configuration;
                const bool connectionChanged = old.host() != configuration.host() ||
                    old.port() != configuration.port() || old.useSsl() != configuration.useSsl() ||
                    old.endpointPrefix() != configuration.endpointPrefix() ||
                    old.authenticationEnabled() != configuration.authenticationEnabled() ||
                    old.apiKey() != configuration.apiKey() ||
                    old.customHeaders() != configuration.customHeaders() ||
                    old.verifySslCertificate() != configuration.verifySslCertificate();
                const bool enabledChanged = old.isEnabled() != configuration.isEnabled();
                state->configuration = configuration;
                state->details.setConfiguration(configuration);
                if (connectionChanged || enabledChanged) {
                    qCInfo(lcMetadataRefreshController)
                        << "DSSP service connection settings changed"
                        << "service=" << configuration.name()
                        << "connectionChanged=" << connectionChanged
                        << "enabledChanged=" << enabledChanged;
                    ++state->generation;
                    state->pending = false;
                    state->pendingIncludesMetadata = false;
                    if (state->active)
                        d->cancelActiveRequests(state);
                }
                if (connectionChanged) {
                    // Keep the last-good metadata associated with this stable service id while
                    // the edited endpoint is being refreshed. Disabled services are excluded
                    // from Core registration without discarding their cached metadata.
                    state->details.setMetadataStale(
                        state->details.lastMetadataRefresh().isValid());
                    state->details.setMaximumApiVersion(0);
                    state->details.setSelectedApiVersion(0);
                    state->healthError.clear();
                    d->syncError(state);
                    state->details.setHealthStatus(configuration.isEnabled()
                                                       ? ServiceInstanceDetails::Unknown
                                                       : ServiceInstanceDetails::Disabled);
                } else if (!configuration.isEnabled()) {
                    state->healthError.clear();
                    d->syncError(state);
                    state->details.setHealthStatus(ServiceInstanceDetails::Disabled);
                } else if (!old.isEnabled()) {
                    state->details.setMetadataStale(
                        state->details.lastMetadataRefresh().isValid());
                    state->details.setHealthStatus(ServiceInstanceDetails::Unknown);
                }
                if (state->timer)
                    state->timer->stop();
            }
            d->emitDetails(state, true);
            d->scheduleTimer(state);
        }

        for (auto it = d->states.begin(); it != d->states.end();) {
            if (currentIds.contains(it.key())) {
                ++it;
                continue;
            }
            const auto state = it.value();
            qCInfo(lcMetadataRefreshController)
                << "Removing DSSP service from refresh controller"
                << "service=" << state->configuration.name();
            state->removed = true;
            ++state->generation;
            if (state->timer)
                state->timer->deleteLater();
            if (state->active) {
                d->cancelActiveRequests(state);
                d->setActive(state, false);
                state->activeGeneration = 0;
            }
            it = d->states.erase(it);
        }
        d->order = newOrder;
        Q_EMIT metadataChanged();
    }

    QList<ServiceInstanceDetails> MetadataRefreshController::serviceDetails() const {
        QList<ServiceInstanceDetails> result;
        result.reserve(d->order.size());
        for (const auto &id : d->order) {
            const auto state = d->states.value(id);
            if (state)
                result.append(state->details);
        }
        return result;
    }

    std::optional<ServiceInstanceDetails>
    MetadataRefreshController::serviceDetails(const QUuid &serviceId) const {
        const auto state = d->states.value(serviceId);
        if (!state)
            return std::nullopt;
        return state->details;
    }

    bool MetadataRefreshController::isRefreshing() const {
        return d->activeCount > 0;
    }

    void MetadataRefreshController::start() {
        if (d->started) {
            qCDebug(lcMetadataRefreshController) << "Refresh controller is already started";
            return;
        }
        qCInfo(lcMetadataRefreshController) << "Starting DSSP refresh controller with"
                                            << d->order.size() << "configured service(s)";
        d->started = true;
        d->stopping = false;
        refreshAll();
    }

    void MetadataRefreshController::stop() {
        if (d->stopping)
            return;
        qCInfo(lcMetadataRefreshController) << "Stopping DSSP refresh controller";
        d->stopping = true;
        d->started = false;
        for (const auto &state : std::as_const(d->states)) {
            ++state->generation;
            state->pending = false;
            if (state->timer)
                state->timer->stop();
            if (state->active) {
                d->cancelActiveRequests(state);
                d->setActive(state, false);
                state->activeGeneration = 0;
            }
        }
    }

    void MetadataRefreshController::refreshAll() {
        if (d->stopping) {
            qCDebug(lcMetadataRefreshController) << "Refresh-all request ignored while stopping";
            return;
        }
        qCInfo(lcMetadataRefreshController) << "Refresh-all requested for"
                                            << d->order.size() << "DSSP service(s)";
        for (const auto &id : std::as_const(d->order)) {
            const auto state = d->states.value(id);
            if (state)
                d->request(state, true);
        }
    }

}
