// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisTaskManager.h"
#include "SynthesisTaskManager_p.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>
#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QSettings>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/runtimeinterface.h>

#include <synth/ServiceTypes.h>
#include <synth/SynthInterface.h>
#include <synth/SynthesisTask.h>
#include <synth/internal/SynthesisTaskCodec.h>
#include <synth/private/SynthesisTask_p.h>

namespace Synth {

    Q_STATIC_LOGGING_CATEGORY(lcSynthesisTaskManager, "diffscope.synth.taskmanager")

    using namespace Internal::TaskCodec;

    namespace {

        bool parametersAreNormalized(const QMap<QString, SynthesisParameter> &parameters) {
            for (auto it = parameters.cbegin(); it != parameters.cend(); ++it) {
                if (!std::ranges::all_of(it->values, [](double value) {
                        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
                    })) {
                    return false;
                }
            }
            return true;
        }

    }

    SynthesisTaskManagerPrivate::SynthesisTaskManagerPrivate(SynthesisTaskManager *q)
        : q_ptr(q),
          apiClient(new Internal::Api::ApiClient(q)),
          networkManager(new QNetworkAccessManager(q)) {
        const auto runtimeData = Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::RuntimeData);
        diagnosticsRoot = QDir(runtimeData).filePath(QStringLiteral("synth/diagnostics-v1"));
        QDir().mkpath(diagnosticsRoot);
        reloadDiagnosticsSettings();
        trimDiagnostics();
        if (auto interface = SynthInterface::instance()) {
            QObject::connect(interface, &SynthInterface::serviceInstancesChanged, q, [this] { pump(); });
            QObject::connect(interface, &SynthInterface::serviceInstanceDetailsChanged, q, [this](const QUuid &) { pump(); });
        }
        QTimer::singleShot(0, q, [this] { pump(); });
    }

    QString SynthesisTaskManagerPrivate::counterKey(const QUuid &serviceId, SynthesisTaskType type) {
        return serviceId.toString(QUuid::WithoutBraces) + u':' + typeName(type);
    }

    bool SynthesisTaskManagerPrivate::providesContext(const ServiceInstanceDetails &details, const SynthesisContext &context) {
        const auto architectures = details.metadata().architectures();
        const auto architecture = std::ranges::find_if(architectures, [&context](const ArchitectureMetadata &candidate) {
            return candidate.id() == context.architectureId;
        });
        if (architecture == architectures.cend()) {
            return false;
        }
        const auto singers = details.metadata().singers();
        return std::ranges::all_of(context.singers, [&singers, &context](const SynthesisSinger &requested) {
            return std::ranges::any_of(singers, [&requested, &context](const SingerMetadata &singer) {
                return singer.id() == requested.id && singer.architectureId() == context.architectureId;
            });
        });
    }

    SynthesisTaskManagerPrivate::ServiceResolution SynthesisTaskManagerPrivate::resolveService(const SynthesisContext &context) const {
        auto interface = SynthInterface::instance();
        if (!interface || context.architectureId.isEmpty() || context.singers.isEmpty()) {
            return {};
        }

        std::optional<ServiceInstanceConfiguration> waitingService;
        bool resolutionPending{};
        for (const auto &configuration : interface->serviceInstances()) {
            if (!configuration.isEnabled()) {
                continue;
            }
            const auto details = interface->serviceInstanceDetails(configuration.id());
            const bool matches = providesContext(details, context);
            if (details.healthStatus() == ServiceInstanceDetails::Healthy && matches) {
                return {ServiceAvailability::Ready, configuration};
            }
            if ((details.healthStatus() == ServiceInstanceDetails::Unknown ||
                 details.healthStatus() == ServiceInstanceDetails::Checking) ||
                (details.healthStatus() == ServiceInstanceDetails::Error && matches)) {
                resolutionPending = true;
                if (matches && !waitingService) {
                    waitingService = configuration;
                }
            }
        }
        if (resolutionPending) {
            return {ServiceAvailability::Waiting, waitingService};
        }
        return {};
    }

    ArchitectureMetadata SynthesisTaskManagerPrivate::architectureMetadata(const ServiceInstanceConfiguration &service, const QString &architectureId) const {
        const auto details = SynthInterface::instance()->serviceInstanceDetails(service.id());
        for (const auto &architecture : details.metadata().architectures()) {
            if (architecture.id() == architectureId) {
                return architecture;
            }
        }
        return {};
    }

    QJsonObject SynthesisTaskManagerPrivate::cacheEnvelope(const ServiceInstanceConfiguration &service, const SynthesisTaskRequest &request, const QString &environmentTag) const {
        const auto details = SynthInterface::instance()->serviceInstanceDetails(service.id());
        return {
            {QStringLiteral("cacheFormatVersion"), 1},
            {QStringLiteral("taskType"), typeName(request.type)},
            {QStringLiteral("serviceId"), service.id().toString(QUuid::WithoutBraces)},
            {QStringLiteral("serviceUrl"), service.baseUrl().toString(QUrl::FullyEncoded)},
            {QStringLiteral("apiVersion"), details.selectedApiVersion()},
            {QStringLiteral("environmentTag"), environmentTag},
            {QStringLiteral("request"), requestToJson(request)},
        };
    }

    QByteArray SynthesisTaskManagerPrivate::taskCacheKey(const ServiceInstanceConfiguration &service, const SynthesisTaskRequest &request, const QString &environmentTag) const {
        return typeName(request.type).toLatin1() + '-' + digest(cacheEnvelope(service, request, environmentTag));
    }

    QByteArray SynthesisTaskManagerPrivate::parameterCacheKey(const ServiceInstanceConfiguration &service, const SynthesisTaskRequest &request, const QString &environmentTag, const QString &target, const QMap<QString, SynthesisParameter> &parameters, const QSet<QString> &unavailableParameters) const {
        auto envelope = cacheEnvelope(service, request, environmentTag);
        auto requestJson = envelope.value(QStringLiteral("request")).toObject();
        auto score = scoreCommonToJson(request.score);
        QJsonObject dependencies;
        const auto architecture = architectureMetadata(service, request.context.architectureId);
        QStringList dependsOn;
        for (const auto &metadata : architecture.parameters()) {
            if (metadata.id() == target) {
                dependsOn = metadata.dependsOn();
                break;
            }
        }
        for (const auto &dependency : dependsOn) {
            if (unavailableParameters.contains(dependency)) {
                return {};
            }
            const auto it = parameters.constFind(dependency);
            if (it == parameters.cend() || it->values.isEmpty()) {
                return {};
            }
            dependencies.insert(dependency, parameterToJson(it.value()));
        }
        score.insert(QStringLiteral("dependencies"), dependencies);
        score.insert(QStringLiteral("target"), target);
        requestJson.insert(QStringLiteral("score"), score);
        requestJson.insert(QStringLiteral("type"), QStringLiteral("parameter-node"));
        envelope.insert(QStringLiteral("request"), requestJson);
        return typeName(SynthesisTaskType::Parameter).toLatin1() + '-' + digest(envelope);
    }

    void SynthesisTaskManagerPrivate::setService(SynthesisTask *task, const ServiceInstanceConfiguration &service) {
        auto d = SynthesisTaskPrivate::get(task);
        if (d->serviceId == service.id() && d->serviceName == service.name()) {
            return;
        }
        const auto previousServiceId = d->serviceId;
        d->serviceId = service.id();
        d->serviceName = service.name();
        Q_EMIT task->serviceChanged();
        Q_EMIT q_ptr->taskChanged(task);
        if (!previousServiceId.isNull()) {
            Q_EMIT q_ptr->serviceTaskCountsChanged(previousServiceId);
        }
        if (!d->serviceId.isNull() && d->serviceId != previousServiceId) {
            Q_EMIT q_ptr->serviceTaskCountsChanged(d->serviceId);
        }
    }

    void SynthesisTaskManagerPrivate::setState(SynthesisTask *task, SynthesisTask::State state, const QString &error) {
        auto d = SynthesisTaskPrivate::get(task);
        if (d->state == state && d->errorMessage == error) {
            return;
        }
        d->state = state;
        d->errorMessage = error;
        if (state == SynthesisTask::Running) {
            d->startedAt = QDateTime::currentDateTimeUtc();
        }
        if (state == SynthesisTask::Succeeded || state == SynthesisTask::Failed || state == SynthesisTask::Canceled) {
            d->finishedAt = QDateTime::currentDateTimeUtc();
        }
        Q_EMIT task->stateChanged();
        Q_EMIT q_ptr->taskChanged(task);
        Q_EMIT q_ptr->taskCountsChanged();
        if (!d->serviceId.isNull()) {
            Q_EMIT q_ptr->serviceTaskCountsChanged(d->serviceId);
        }
        if (task->isFinished()) {
            Q_EMIT task->finished();
        }
    }

    bool SynthesisTaskManagerPrivate::canStart(const ServiceInstanceConfiguration &service, SynthesisTaskType type) const {
        return activeByService.value(service.id()) < service.globalConcurrency() &&
               activeByServiceAndType.value(counterKey(service.id(), type)) < service.taskConcurrency();
    }

    void SynthesisTaskManagerPrivate::pump() {
        if (shuttingDown) {
            return;
        }
        std::stable_sort(queue.begin(), queue.end(), [](SynthesisTask *left, SynthesisTask *right) {
            return left->priority() > right->priority();
        });
        for (qsizetype index = 0; index < queue.size();) {
            auto task = queue.at(index);
            const auto resolution = resolveService(task->request().context);
            if (resolution.service) {
                setService(task, *resolution.service);
            }
            if (resolution.availability == ServiceAvailability::Waiting) {
                ++index;
                continue;
            }
            if (resolution.availability == ServiceAvailability::Unavailable || !resolution.service) {
                queue.removeAt(index);
                setState(task, SynthesisTask::Failed, Synth::SynthesisTaskManager::tr("No healthy synthesis service supports the selected architecture and all required singers"));
                continue;
            }
            if (!canStart(*resolution.service, task->type())) {
                ++index;
                continue;
            }
            queue.removeAt(index);
            start(task, *resolution.service);
        }
    }

    void SynthesisTaskManagerPrivate::start(SynthesisTask *task, const ServiceInstanceConfiguration &service) {
        diagnosticExchanges.remove(task);
        auto taskPrivate = SynthesisTaskPrivate::get(task);
        if (!taskPrivate->diagnostics.isEmpty() || !taskPrivate->diagnosticFilePath.isEmpty()) {
            taskPrivate->diagnostics.clear();
            taskPrivate->diagnosticFilePath.clear();
            Q_EMIT task->diagnosticsChanged();
        }
        setState(task, SynthesisTask::Running);
        ++activeByService[service.id()];
        ++activeByServiceAndType[counterKey(service.id(), task->type())];
        obtainEnvironmentTag(task, service);
    }

    void SynthesisTaskManagerPrivate::releaseSlot(SynthesisTask *task) {
        const auto serviceId = task->serviceInstanceId();
        activeByService[serviceId] = std::max(0, activeByService.value(serviceId) - 1);
        const auto key = counterKey(serviceId, task->type());
        activeByServiceAndType[key] = std::max(0, activeByServiceAndType.value(key) - 1);
        cancelFunctions.remove(task);
        QTimer::singleShot(0, q_ptr, [this] { pump(); });
    }

    void SynthesisTaskManagerPrivate::complete(SynthesisTask *task, SynthesisTaskResult result) {
        if (task->state() != SynthesisTask::Running) {
            return;
        }
        auto taskPrivate = SynthesisTaskPrivate::get(task);
        taskPrivate->result = std::move(result);
        diagnosticExchanges.remove(task);
        if (!taskPrivate->diagnostics.isEmpty()) {
            taskPrivate->diagnostics.clear();
            Q_EMIT task->diagnosticsChanged();
        }
        setState(task, SynthesisTask::Succeeded);
        releaseSlot(task);
    }

    void SynthesisTaskManagerPrivate::fail(SynthesisTask *task, const QString &message) {
        if (task->state() != SynthesisTask::Running) {
            return;
        }
        publishDiagnostics(task);
        persistDiagnostics(task, message);
        setState(task, SynthesisTask::Failed, message);
        releaseSlot(task);
    }

    void SynthesisTaskManagerPrivate::finishCanceled(SynthesisTask *task) {
        if (task->state() != SynthesisTask::Running) {
            return;
        }
        auto taskPrivate = SynthesisTaskPrivate::get(task);
        diagnosticExchanges.remove(task);
        if (!taskPrivate->diagnostics.isEmpty()) {
            taskPrivate->diagnostics.clear();
            Q_EMIT task->diagnosticsChanged();
        }
        setState(task, SynthesisTask::Canceled);
        releaseSlot(task);
    }

    void SynthesisTaskManagerPrivate::requeue(SynthesisTask *task, const std::optional<ServiceInstanceConfiguration> &service) {
        if (!task || task->state() != SynthesisTask::Running) {
            return;
        }
        queue.append(task);
        diagnosticExchanges.remove(task);
        setState(task, SynthesisTask::Queued);
        releaseSlot(task);
        if (service) {
            setService(task, *service);
        }
    }

    void SynthesisTaskManagerPrivate::appendExchanges(SynthesisTask *task, const QList<Internal::Api::ApiExchange> &exchanges) {
        if (!task || task->state() != SynthesisTask::Running || exchanges.isEmpty()) {
            return;
        }
        diagnosticExchanges[task] += exchanges;
    }

    void SynthesisTaskManagerPrivate::appendExchange(SynthesisTask *task, const Internal::Api::ApiExchange &exchange) {
        if (!task || task->state() != SynthesisTask::Running) {
            return;
        }
        diagnosticExchanges[task].append(exchange);
    }

    void SynthesisTaskManagerPrivate::publishDiagnostics(SynthesisTask *task) {
        const auto exchanges = diagnosticExchanges.take(task);
        if (exchanges.isEmpty()) {
            return;
        }
        auto taskPrivate = SynthesisTaskPrivate::get(task);
        for (const auto &exchange : exchanges) {
            taskPrivate->diagnostics.append(QVariantMap{
                {QStringLiteral("requestId"), QString::number(exchange.requestId)},
                {QStringLiteral("serviceInstanceId"), exchange.serviceInstanceId.toString(QUuid::WithoutBraces)},
                {QStringLiteral("method"), QString::fromLatin1(exchange.method)},
                {QStringLiteral("url"), exchange.url.toString(QUrl::FullyEncoded)},
                {QStringLiteral("attempt"), exchange.attempt},
                {QStringLiteral("statusCode"), exchange.httpStatusCode},
                {QStringLiteral("networkErrorCode"), exchange.networkErrorCode},
                {QStringLiteral("startedAt"), exchange.startedAt.toString(Qt::ISODateWithMs)},
                {QStringLiteral("finishedAt"), exchange.finishedAt.toString(Qt::ISODateWithMs)},
                {QStringLiteral("requestBody"), QString::fromUtf8(exchange.requestBody)},
                {QStringLiteral("responseBody"), QString::fromUtf8(exchange.responseBody)},
                {QStringLiteral("errorMessage"), exchange.errorMessage},
            });
        }
        Q_EMIT task->diagnosticsChanged();
    }

    QJsonObject SynthesisTaskManagerPrivate::diagnosticDocument(SynthesisTask *task, const QString &message) const {
        const auto taskPrivate = SynthesisTaskPrivate::get(task);
        return {
            {QStringLiteral("version"), 1},
            {QStringLiteral("taskId"), task->id().toString(QUuid::WithoutBraces)},
            {QStringLiteral("taskType"), typeName(task->type())},
            {QStringLiteral("taskName"), task->displayName()},
            {QStringLiteral("serviceInstanceId"), task->serviceInstanceId().toString(QUuid::WithoutBraces)},
            {QStringLiteral("failedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
            {QStringLiteral("errorMessage"), message},
            {QStringLiteral("exchanges"), QJsonArray::fromVariantList(taskPrivate->diagnostics)},
        };
    }

    void SynthesisTaskManagerPrivate::persistDiagnostics(SynthesisTask *task, const QString &message) {
        auto taskPrivate = SynthesisTaskPrivate::get(task);
        if (!QDir().mkpath(diagnosticsRoot)) {
            qCWarning(lcSynthesisTaskManager) << "Could not create synthesis diagnostics directory" << diagnosticsRoot;
            return;
        }
        const auto fileName = QStringLiteral("%1-%2.json")
                                  .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")))
                                  .arg(task->id().toString(QUuid::WithoutBraces));
        const auto path = QDir(diagnosticsRoot).filePath(fileName);
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(lcSynthesisTaskManager) << "Could not open synthesis diagnostics file" << path;
            return;
        }
        const auto bytes = QJsonDocument(diagnosticDocument(task, message)).toJson(QJsonDocument::Indented);
        if (file.write(bytes) != bytes.size() || !file.commit()) {
            qCWarning(lcSynthesisTaskManager) << "Could not write synthesis diagnostics file" << path;
            return;
        }
        trimDiagnostics();
        if (QFileInfo::exists(path)) {
            taskPrivate->diagnosticFilePath = path;
            Q_EMIT task->diagnosticsChanged();
        }
    }

    void SynthesisTaskManagerPrivate::trimDiagnostics() {
        auto files = QDir(diagnosticsRoot).entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Time | QDir::Reversed);
        const auto now = QDateTime::currentDateTimeUtc();
        qint64 total{};
        for (auto it = files.begin(); it != files.end();) {
            if (diagnosticsExpiryDays > 0 && it->lastModified().toUTC().daysTo(now) > diagnosticsExpiryDays) {
                QFile::remove(it->absoluteFilePath());
                it = files.erase(it);
            } else {
                total += it->size();
                ++it;
            }
        }
        qsizetype remaining = files.size();
        for (const auto &file : files) {
            if (diagnosticsMaximumBytes <= 0 || total <= diagnosticsMaximumBytes || remaining <= 1) {
                break;
            }
            const auto size = file.size();
            if (QFile::remove(file.absoluteFilePath())) {
                total -= size;
                --remaining;
            }
        }
        for (auto task : tasks) {
            auto taskPrivate = SynthesisTaskPrivate::get(task);
            if (!taskPrivate->diagnosticFilePath.isEmpty() &&
                !QFileInfo::exists(taskPrivate->diagnosticFilePath)) {
                taskPrivate->diagnosticFilePath.clear();
                Q_EMIT task->diagnosticsChanged();
            }
        }
    }

    void SynthesisTaskManagerPrivate::reloadDiagnosticsSettings() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synth"));
        diagnosticsMaximumBytes = settings->value(QStringLiteral("diagnosticsMaximumBytes"), qint64(256) * 1024 * 1024).toLongLong();
        diagnosticsExpiryDays = settings->value(QStringLiteral("diagnosticsExpiryDays"), 7).toInt();
        settings->endGroup();
    }

    using namespace Internal::Api;

    namespace {

        QString apiErrorText(const ApiError &error) {
            if (!error.message.isEmpty()) {
                return error.message;
            }
            if (error.httpStatusCode) {
                return Synth::SynthesisTaskManager::tr("The synthesis service returned HTTP status %1")
                    .arg(error.httpStatusCode);
            }
            return Synth::SynthesisTaskManager::tr("The synthesis service request failed");
        }

    }

    QByteArray SynthesisTaskManagerPrivate::environmentKey(const ServiceInstanceConfiguration &service, const SynthesisContext &context) const {
        const auto details = SynthInterface::instance()->serviceInstanceDetails(service.id());
        return digest({
            {QStringLiteral("serviceId"), service.id().toString(QUuid::WithoutBraces)},
            {QStringLiteral("serviceUrl"), service.baseUrl().toString(QUrl::FullyEncoded)},
            {QStringLiteral("apiVersion"), details.selectedApiVersion()},
            {QStringLiteral("context"), contextToJson(context)},
        });
    }

    void SynthesisTaskManagerPrivate::obtainEnvironmentTag(SynthesisTask *task, const ServiceInstanceConfiguration &service) {
        const auto key = environmentKey(service, task->request().context);
        const auto cached = environmentTags.constFind(key);
        if (cached != environmentTags.cend() && cached->expiresAt > QDateTime::currentDateTimeUtc()) {
            execute(task, service, cached->tag);
            return;
        }

        cancelFunctions.insert(task, [] {});
        auto pending = pendingEnvironmentTags.find(key);
        if (pending != pendingEnvironmentTags.end()) {
            pending->tasks.append(task);
            return;
        }
        pendingEnvironmentTags.insert(key, {{task}});
        requestEnvironmentTag(key, service, task->request().context);
    }

    void SynthesisTaskManagerPrivate::requestEnvironmentTag(const QByteArray &key, const ServiceInstanceConfiguration &service, const SynthesisContext &context) {
        V1::EnvTagRequest request;
        request.context = multiContext(context);
        auto future = apiClient->createEnvironmentTag(service, request);
        auto watcher = new QFutureWatcher<ApiResult<V1::EnvTagResponse>>(q_ptr);
        QObject::connect(watcher, &QFutureWatcherBase::finished, q_ptr, [this, watcher, key] {
            const auto pending = pendingEnvironmentTags.take(key);
            if (shuttingDown) {
                watcher->deleteLater();
                for (const auto &task : pending.tasks) {
                    if (task && task->state() == SynthesisTask::Running) {
                        finishCanceled(task);
                    }
                }
                return;
            }
            QList<QPointer<SynthesisTask>> validTasks;
            for (const auto &task : pending.tasks) {
                if (!task || task->state() != SynthesisTask::Running) {
                    continue;
                }
                const auto resolution = resolveService(task->request().context);
                if (resolution.availability == ServiceAvailability::Unavailable) {
                    cancelFunctions.remove(task);
                    fail(task, Synth::SynthesisTaskManager::tr("No healthy synthesis service supports the selected architecture and all required singers"));
                    continue;
                }
                if (resolution.availability == ServiceAvailability::Waiting || !resolution.service ||
                    environmentKey(*resolution.service, task->request().context) != key) {
                    cancelFunctions.remove(task);
                    requeue(task, resolution.service);
                    continue;
                }
                validTasks.append(task);
            }
            if (watcher->future().isCanceled()) {
                watcher->deleteLater();
                for (const auto &task : validTasks) {
                    if (task) {
                        finishCanceled(task);
                    }
                }
                return;
            }

            const auto result = watcher->result();
            watcher->deleteLater();
            for (const auto &task : validTasks) {
                if (task) {
                    appendExchanges(task, result.exchanges());
                }
            }
            if (!result) {
                for (const auto &task : validTasks) {
                    if (task) {
                        cancelFunctions.remove(task);
                        fail(task, apiErrorText(result.error()));
                    }
                }
                return;
            }

            const auto tag = result.value().envTag;
            if (validTasks.isEmpty()) {
                return;
            }
            environmentTags.insert(key, {
                                            tag,
                                            QDateTime::currentDateTimeUtc().addSecs(std::max(1, cache.environmentTagTtlSeconds())),
                                        });
            for (const auto &task : validTasks) {
                if (!task || task->state() != SynthesisTask::Running) {
                    continue;
                }
                const auto resolution = resolveService(task->request().context);
                if (resolution.availability != ServiceAvailability::Ready || !resolution.service ||
                    environmentKey(*resolution.service, task->request().context) != key) {
                    requeue(task, resolution.service);
                    continue;
                }
                cancelFunctions.remove(task);
                execute(task, *resolution.service, tag);
            }
        });
        watcher->setFuture(future);
    }

    void SynthesisTaskManagerPrivate::execute(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag) {
        if (task->state() != SynthesisTask::Running) {
            return;
        }
        if (task->type() == SynthesisTaskType::Parameter) {
            executeParameter(task, service, environmentTag);
            return;
        }
        if (task->type() == SynthesisTaskType::Audio &&
            !parametersAreNormalized(task->request().score.parameters)) {
            fail(task, SynthesisTaskManager::tr("Audio request contains a non-normalized parameter value"));
            return;
        }
        const auto key = taskCacheKey(service, task->request(), environmentTag);
        SynthesisTaskResult cached;
        if (task->options().readCache && cache.read(task->type(), key, &cached)) {
            complete(task, std::move(cached));
            return;
        }
        switch (task->type()) {
            case SynthesisTaskType::Pronunciation:
                executePronunciation(task, service, environmentTag, key);
                break;
            case SynthesisTaskType::Phoneme:
                executePhoneme(task, service, environmentTag, key);
                break;
            case SynthesisTaskType::Duration:
                executeDuration(task, service, environmentTag, key);
                break;
            case SynthesisTaskType::Audio:
                executeAudio(task, service, environmentTag, key);
                break;
            case SynthesisTaskType::Parameter:
                break;
        }
    }

    void SynthesisTaskManagerPrivate::maybeWriteAndComplete(SynthesisTask *task, const QByteArray &key, SynthesisTaskResult result) {
        if (task->options().writeCache) {
            cache.write(task->type(), key, result);
        }
        complete(task, std::move(result));
    }

    void SynthesisTaskManagerPrivate::executePronunciation(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &, const QByteArray &key) {
        V1::PronunciationRequest request;
        request.context = singleContext(task->request().context);
        for (const auto &note : task->request().lyricNotes) {
            request.input.notes.append({note.lyric, note.language});
        }
        watch(task, apiClient->synthesizePronunciation(service, request), [this, task, key](ApiResult<V1::PronunciationResponse> response) {
            if (!response) {
                fail(task, apiErrorText(response.error()));
                return;
            }
            SynthesisTaskResult result;
            for (const auto &note : response.value().output.notes) {
                result.pronunciations.append({note.pronunciation, note.candidates});
            }
            maybeWriteAndComplete(task, key, std::move(result));
        });
    }

    void SynthesisTaskManagerPrivate::executePhoneme(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &, const QByteArray &key) {
        V1::PhonemeRequest request;
        request.context = singleContext(task->request().context);
        for (const auto &note : task->request().pronunciationNotes) {
            request.input.notes.append({note.pronunciation, note.language});
        }
        watch(task, apiClient->synthesizePhoneme(service, request), [this, task, key](ApiResult<V1::PhonemeResponse> response) {
            if (!response) {
                fail(task, apiErrorText(response.error()));
                return;
            }
            SynthesisTaskResult result;
            const auto input = task->request().pronunciationNotes;
            for (int noteIndex = 0; noteIndex < response.value().output.notes.size(); ++noteIndex) {
                QList<SynthesisPhoneme> phonemes;
                const auto language = noteIndex < input.size() ? input.at(noteIndex).language : QString{};
                for (const auto &phoneme : response.value().output.notes.at(noteIndex).phonemes) {
                    phonemes.append({phoneme.token, phoneme.onset, language, 0.0});
                }
                result.phonemes.append(phonemes);
            }
            maybeWriteAndComplete(task, key, std::move(result));
        });
    }

    void SynthesisTaskManagerPrivate::executeDuration(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &, const QByteArray &key) {
        V1::DurationRequest request;
        request.context = multiContext(task->request().context);
        request.input.pieceDuration = task->request().score.pieceDuration;
        request.input.mix = mixToDto(task->request().score.mix);
        request.input.mixSampleRate = task->request().score.mixSampleRate;
        for (const auto &note : task->request().score.notes) {
            V1::DurationNote converted;
            converted.position = {note.gap, note.duration};
            converted.cent = note.cent;
            converted.pronunciation = note.pronunciation;
            converted.language = note.language;
            for (const auto &phoneme : note.phonemes) {
                converted.phonemes.append({phoneme.token, phoneme.onset, phoneme.language});
            }
            request.input.notes.append(converted);
        }
        watch(task, apiClient->synthesizeDuration(service, request), [this, task, key](ApiResult<V1::DurationResponse> response) {
            if (!response) {
                fail(task, apiErrorText(response.error()));
                return;
            }
            SynthesisTaskResult result;
            const auto inputNotes = task->request().score.notes;
            for (int noteIndex = 0; noteIndex < response.value().output.notes.size(); ++noteIndex) {
                QList<SynthesisPhoneme> phonemes = noteIndex < inputNotes.size()
                                                       ? inputNotes.at(noteIndex).phonemes
                                                       : QList<SynthesisPhoneme>{};
                const auto starts = response.value().output.notes.at(noteIndex).phonemes;
                for (int index = 0; index < std::min(phonemes.size(), starts.size()); ++index) {
                    phonemes[index].start = starts.at(index).start;
                }
                result.phonemes.append(phonemes);
            }
            maybeWriteAndComplete(task, key, std::move(result));
        });
    }

    void SynthesisTaskManagerPrivate::executeParameter(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag) {
        auto requestModel = task->request();
        if (!parametersAreNormalized(requestModel.score.parameters)) {
            fail(task, SynthesisTaskManager::tr("Parameter request contains a non-normalized value"));
            return;
        }
        SynthesisTaskResult combined;
        QStringList pending = requestModel.score.requestedParameters;
        QSet<QString> unavailableParameters(pending.cbegin(), pending.cend());
        bool progressed = true;
        while (progressed && !pending.isEmpty()) {
            progressed = false;
            QStringList next;
            for (const auto &target : pending) {
                const auto key = parameterCacheKey(service, requestModel, environmentTag, target, requestModel.score.parameters, unavailableParameters);
                SynthesisTaskResult cached;
                if (!key.isEmpty() && task->options().readCache && cache.read(SynthesisTaskType::Parameter, key, &cached) && cached.parameters.contains(target)) {
                    const auto parameter = cached.parameters.value(target);
                    requestModel.score.parameters.insert(target, parameter);
                    combined.parameters.insert(target, parameter);
                    combined.fromCache = true;
                    unavailableParameters.remove(target);
                    progressed = true;
                } else {
                    next.append(target);
                }
            }
            pending = next;
        }
        if (pending.isEmpty()) {
            complete(task, std::move(combined));
            return;
        }
        requestModel.score.requestedParameters = pending;
        for (const auto &target : pending) {
            if (!requestModel.score.parameters.contains(target)) {
                requestModel.score.parameters.insert(target, {{}, 100.0});
            }
        }

        V1::ParameterRequest request;
        request.context = multiContext(requestModel.context);
        request.input.pieceDuration = requestModel.score.pieceDuration;
        request.input.notes = parameterNotes(requestModel.score);
        request.input.mix = mixToDto(requestModel.score.mix);
        request.input.mixSampleRate = requestModel.score.mixSampleRate;
        for (auto it = requestModel.score.parameters.cbegin(); it != requestModel.score.parameters.cend(); ++it) {
            V1::Parameter parameter;
            parameter.values = it->values;
            parameter.sampleRate = it->sampleRate;
            if (pending.contains(it.key())) {
                parameter.retake = V1::ParameterRetake{
                    0,
                    std::max(0, static_cast<int>(std::ceil(requestModel.score.pieceDuration * it->sampleRate))),
                };
            }
            request.input.parameters.values.insert(it.key(), parameter);
        }
        watch(task, apiClient->synthesizeParameter(service, request), [this, task, service, environmentTag, requestModel, combined = std::move(combined)](ApiResult<V1::ParameterResponse> response) mutable {
            if (!response) {
                fail(task, apiErrorText(response.error()));
                return;
            }
            auto available = requestModel.score.parameters;
            for (auto it = response.value().output.parameters.cbegin(); it != response.value().output.parameters.cend(); ++it) {
                const SynthesisParameter parameter{it->values, it->sampleRate};
                available.insert(it.key(), parameter);
                combined.parameters.insert(it.key(), parameter);
            }
            if (task->options().writeCache) {
                for (auto it = combined.parameters.cbegin(); it != combined.parameters.cend(); ++it) {
                    if (!requestModel.score.requestedParameters.contains(it.key())) {
                        continue;
                    }
                    const auto key = parameterCacheKey(service, requestModel, environmentTag, it.key(), available);
                    if (!key.isEmpty()) {
                        SynthesisTaskResult node;
                        node.parameters.insert(it.key(), it.value());
                        cache.write(SynthesisTaskType::Parameter, key, node);
                    }
                }
            }
            complete(task, std::move(combined));
        });
    }

    void SynthesisTaskManagerPrivate::executeAudio(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &, const QByteArray &key) {
        V1::AudioRequest request;
        request.context = multiContext(task->request().context);
        request.input.pieceDuration = task->request().score.pieceDuration;
        request.input.notes = parameterNotes(task->request().score);
        request.input.mix = mixToDto(task->request().score.mix);
        request.input.mixSampleRate = task->request().score.mixSampleRate;
        for (auto it = task->request().score.parameters.cbegin(); it != task->request().score.parameters.cend(); ++it) {
            V1::AudioParameter parameter;
            parameter.values = it->values;
            parameter.sampleRate = it->sampleRate;
            request.input.parameters.values.insert(it.key(), parameter);
        }
        watch(task, apiClient->synthesizeAudio(service, request), [this, task, service, key](ApiResult<V1::AudioResponse> response) {
            if (!response) {
                fail(task, apiErrorText(response.error()));
                return;
            }
            materializeAudio(task, service, key, response.value().output.audioUrl);
        });
    }

    namespace {

        QString problemDetailsText(const QByteArray &body) {
            QJsonParseError parseError;
            const auto document = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                return {};
            }
            const auto object = document.object();
            const auto title = object.value(QStringLiteral("title")).toString().trimmed();
            const auto detail = object.value(QStringLiteral("detail")).toString().trimmed();
            if (title.isEmpty()) {
                return detail;
            }
            if (detail.isEmpty() || detail == title) {
                return title;
            }
            return title + u'\n' + detail;
        }

        QString audioDownloadErrorText(QNetworkReply *reply, const QByteArray &body) {
            const auto statusAttribute = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            if (!statusAttribute.isValid()) {
                return Synth::SynthesisTaskManager::tr("The synthesis service request failed: %1")
                    .arg(reply->errorString());
            }

            const int status = statusAttribute.toInt();
            const auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute)
                                    .toString()
                                    .trimmed();
            auto message = reason.isEmpty()
                               ? Synth::SynthesisTaskManager::tr("The synthesis service returned HTTP status %1")
                                     .arg(status)
                               : Synth::SynthesisTaskManager::tr("The synthesis service returned HTTP status %1: %2")
                                     .arg(status)
                                     .arg(reason);
            const auto problem = problemDetailsText(body);
            if (!problem.isEmpty()) {
                message += u'\n' + problem;
            } else if (!reply->errorString().isEmpty()) {
                message += u'\n' + reply->errorString();
            }
            return message;
        }

    }

    QString SynthesisTaskManagerPrivate::suffixForMime(const QString &mimeName) const {
        const auto suffixes = QMimeDatabase().mimeTypeForName(mimeName).suffixes();
        return suffixes.isEmpty() ? QStringLiteral(".audio") : u'.' + suffixes.first();
    }

    void SynthesisTaskManagerPrivate::finishAudioBytes(SynthesisTask *task, const QByteArray &key, const QByteArray &bytes, const QString &suffix) {
        if (bytes.isEmpty() || bytes.size() > cache.maximumDownloadBytes()) {
            fail(task, Synth::SynthesisTaskManager::tr("The synthesized audio response is empty or exceeds the configured size limit"));
            return;
        }
        const auto path = cache.audioPath(task->type(), key, suffix, task->options().writeCache);
        if (!cache.writeBytes(path, bytes)) {
            fail(task, Synth::SynthesisTaskManager::tr("Could not store synthesized audio locally"));
            return;
        }
        SynthesisTaskResult result;
        result.audioFilePath = path;
        maybeWriteAndComplete(task, key, std::move(result));
    }

    void SynthesisTaskManagerPrivate::materializeAudio(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QByteArray &key, const QString &location) {
        if (location.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive)) {
            const auto comma = location.indexOf(u',');
            if (comma <= 5) {
                fail(task, Synth::SynthesisTaskManager::tr("The synthesis service returned an invalid audio data URL"));
                return;
            }
            const auto metadata = location.mid(5, comma - 5);
            const auto parts = metadata.split(u';');
            const auto mime = parts.value(0, QStringLiteral("application/octet-stream"));
            const auto payload = location.mid(comma + 1).toLatin1();
            const auto bytes = parts.contains(QStringLiteral("base64"), Qt::CaseInsensitive)
                                   ? QByteArray::fromBase64(payload)
                                   : QByteArray::fromPercentEncoding(payload);
            finishAudioBytes(task, key, bytes, suffixForMime(mime));
            return;
        }
        const QUrl url(location);
        if (!url.isValid() || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
            fail(task, Synth::SynthesisTaskManager::tr("The synthesis service returned an unsupported audio URL"));
            return;
        }
        downloadAudio(task, service, key, url, 0);
    }

    int SynthesisTaskManagerPrivate::effectivePort(const QUrl &url) {
        return url.port(url.scheme() == QStringLiteral("https") ? 443 : 80);
    }

    bool SynthesisTaskManagerPrivate::sameOrigin(const QUrl &left, const QUrl &right) {
        return left.scheme().compare(right.scheme(), Qt::CaseInsensitive) == 0 &&
               left.host().compare(right.host(), Qt::CaseInsensitive) == 0 &&
               effectivePort(left) == effectivePort(right);
    }

    void SynthesisTaskManagerPrivate::downloadAudio(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QByteArray &key, const QUrl &url, int redirectCount) {
        if (redirectCount > 5) {
            fail(task, Synth::SynthesisTaskManager::tr("The synthesized audio URL redirected too many times"));
            return;
        }
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
        const auto timeoutMilliseconds = std::clamp<qint64>(
            static_cast<qint64>(service.requestTimeoutSeconds()) * 1000, 1,
            std::numeric_limits<int>::max()
        );
        request.setTransferTimeout(static_cast<int>(timeoutMilliseconds));
        const bool serviceOrigin = sameOrigin(url, service.baseUrl());
        if (serviceOrigin && service.authenticationEnabled()) {
            request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + service.apiKey().toUtf8());
        }
        if (serviceOrigin) {
            const auto customHeaders = service.parsedCustomHeaders();
            for (auto it = customHeaders.cbegin(); it != customHeaders.cend(); ++it) {
                request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
            }
        }
#if QT_CONFIG(ssl)
        if (serviceOrigin && !service.verifySslCertificate()) {
            auto ssl = request.sslConfiguration();
            ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
            request.setSslConfiguration(ssl);
        }
#endif
        auto reply = networkManager->get(request);
        cancelFunctions.insert(task, [reply] { reply->abort(); });
        QObject::connect(reply, &QNetworkReply::downloadProgress, q_ptr, [this, task, reply](qint64 received, qint64 total) {
            if (received > cache.maximumDownloadBytes() || total > cache.maximumDownloadBytes()) {
                cancelFunctions.remove(task);
                reply->setProperty("synthDownloadTooLarge", true);
                reply->abort();
            }
        });
        const auto startedAt = QDateTime::currentDateTimeUtc();
        QObject::connect(reply, &QNetworkReply::finished, q_ptr, [this, task, service, key, redirectCount, reply, startedAt] {
            cancelFunctions.remove(task);
            if (task->state() == SynthesisTask::Canceled) {
                reply->deleteLater();
                return;
            }
            const auto redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (!redirect.isEmpty()) {
                const auto target = reply->url().resolved(redirect);
                reply->deleteLater();
                downloadAudio(task, service, key, target, redirectCount + 1);
                return;
            }
            if (reply->error() != QNetworkReply::NoError) {
                const auto body = reply->readAll();
                const auto message = reply->property("synthDownloadTooLarge").toBool()
                                         ? Synth::SynthesisTaskManager::tr("The synthesized audio exceeds the configured download size limit")
                                         : audioDownloadErrorText(reply, body);
                Internal::Api::ApiExchange exchange;
                exchange.serviceInstanceId = service.id();
                exchange.method = QByteArrayLiteral("GET");
                exchange.url = reply->url();
                exchange.attempt = redirectCount + 1;
                exchange.httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                exchange.networkErrorCode = static_cast<int>(reply->error());
                exchange.startedAt = startedAt;
                exchange.finishedAt = QDateTime::currentDateTimeUtc();
                exchange.responseBody = body;
                exchange.errorMessage = message;
                appendExchange(task, exchange);
                reply->deleteLater();
                fail(task, message);
                return;
            }
            const auto bytes = reply->readAll();
            const auto mime = reply->header(QNetworkRequest::ContentTypeHeader).toString().section(u';', 0, 0).trimmed();
            QString suffix = QFileInfo(reply->url().path()).suffix();
            suffix = suffix.isEmpty() ? suffixForMime(mime) : u'.' + suffix;
            reply->deleteLater();
            finishAudioBytes(task, key, bytes, suffix);
        });
    }

    SynthesisTaskManager::SynthesisTaskManager(QObject *parent)
        : QObject(parent), d_ptr(new SynthesisTaskManagerPrivate(this)) {
    }

    SynthesisTaskManager::~SynthesisTaskManager() {
        shutdown();
    }

    SynthesisTask *SynthesisTaskManager::enqueue(const SynthesisTaskRequest &request, const SynthesisTaskOptions &options) {
        Q_D(SynthesisTaskManager);
        if (d->shuttingDown) {
            return nullptr;
        }
        auto task = new SynthesisTask(request, options, this);
        d->tasks.append(task);
        d->queue.append(task);
        const auto resolution = d->resolveService(request.context);
        if (resolution.service) {
            d->setService(task, *resolution.service);
        }
        Q_EMIT taskAdded(task);
        Q_EMIT taskCountsChanged();
        QTimer::singleShot(0, this, [d] { d->pump(); });
        return task;
    }

    QList<SynthesisTask *> SynthesisTaskManager::tasks() const {
        Q_D(const SynthesisTaskManager);
        return d->tasks;
    }

    QList<SynthesisTask *> SynthesisTaskManager::tasksForService(const QUuid &serviceInstanceId) const {
        Q_D(const SynthesisTaskManager);
        QList<SynthesisTask *> result;
        for (auto task : d->tasks) {
            if (task->serviceInstanceId() == serviceInstanceId && !task->isFinished()) {
                result.append(task);
            }
        }
        return result;
    }

    int SynthesisTaskManager::runningTaskCount() const {
        Q_D(const SynthesisTaskManager);
        return static_cast<int>(std::ranges::count_if(d->tasks, [](SynthesisTask *task) {
            return task->state() == SynthesisTask::Running;
        }));
    }

    int SynthesisTaskManager::queuedTaskCount() const {
        Q_D(const SynthesisTaskManager);
        return d->queue.size();
    }

    qint64 SynthesisTaskManager::cacheSize() const {
        Q_D(const SynthesisTaskManager);
        return d->cache.size();
    }

    qint64 SynthesisTaskManager::cacheSize(SynthesisTaskType type) const {
        Q_D(const SynthesisTaskManager);
        return d->cache.size(type);
    }

    QString SynthesisTaskManager::diagnosticsDirectory() const {
        Q_D(const SynthesisTaskManager);
        return d->diagnosticsRoot;
    }

    bool SynthesisTaskManager::setPriority(SynthesisTask *task, int priority) {
        Q_D(SynthesisTaskManager);
        if (!task || task->parent() != this || task->state() != SynthesisTask::Queued) {
            return false;
        }
        auto taskPrivate = SynthesisTaskPrivate::get(task);
        if (taskPrivate->options.priority == priority) {
            return true;
        }
        taskPrivate->options.priority = priority;
        Q_EMIT task->priorityChanged();
        Q_EMIT taskChanged(task);
        QTimer::singleShot(0, this, [d] { d->pump(); });
        return true;
    }

    bool SynthesisTaskManager::cancel(SynthesisTask *task) {
        Q_D(SynthesisTaskManager);
        if (!task || task->parent() != this || task->isFinished()) {
            return false;
        }
        if (task->state() == SynthesisTask::Queued) {
            d->queue.removeAll(task);
            d->diagnosticExchanges.remove(task);
            d->setState(task, SynthesisTask::Canceled);
            return true;
        }
        const auto cancel = d->cancelFunctions.value(task);
        if (cancel) {
            d->diagnosticExchanges.remove(task);
            d->setState(task, SynthesisTask::Canceled);
            d->releaseSlot(task);
            cancel();
        }
        return true;
    }

    bool SynthesisTaskManager::removeFinishedTask(SynthesisTask *task) {
        Q_D(SynthesisTaskManager);
        if (!task || task->parent() != this || !task->isFinished()) {
            return false;
        }
        const auto index = d->tasks.indexOf(task);
        if (index < 0) {
            return false;
        }
        d->diagnosticExchanges.remove(task);
        d->cancelFunctions.remove(task);
        d->tasks.removeAt(index);
        Q_EMIT taskRemoved(task);
        task->deleteLater();
        return true;
    }

    void SynthesisTaskManager::clearFinishedTasks() {
        Q_D(SynthesisTaskManager);
        for (qsizetype index = d->tasks.size() - 1; index >= 0; --index) {
            auto task = d->tasks.at(index);
            if (!task->isFinished()) {
                continue;
            }
            removeFinishedTask(task);
        }
    }

    void SynthesisTaskManager::clearFailedTasks() {
        Q_D(SynthesisTaskManager);
        for (qsizetype index = d->tasks.size() - 1; index >= 0; --index) {
            auto task = d->tasks.at(index);
            if (task->state() != SynthesisTask::Failed) {
                continue;
            }
            removeFinishedTask(task);
        }
    }

    void SynthesisTaskManager::clearCache() {
        Q_D(SynthesisTaskManager);
        d->cache.clear();
    }

    void SynthesisTaskManager::clearCache(const QList<SynthesisTaskType> &types) {
        Q_D(SynthesisTaskManager);
        d->cache.clear(types);
    }

    void SynthesisTaskManager::clearDiagnostics() {
        Q_D(SynthesisTaskManager);
        const auto files = QDir(d->diagnosticsRoot).entryInfoList({QStringLiteral("*.json")}, QDir::Files);
        for (const auto &file : files) {
            QFile::remove(file.absoluteFilePath());
        }
        for (auto task : d->tasks) {
            auto taskPrivate = SynthesisTaskPrivate::get(task);
            if (taskPrivate->diagnosticFilePath.isEmpty()) {
                continue;
            }
            taskPrivate->diagnosticFilePath.clear();
            Q_EMIT task->diagnosticsChanged();
        }
    }

    void SynthesisTaskManager::reloadSettings() {
        Q_D(SynthesisTaskManager);
        d->cache.reload();
        d->reloadDiagnosticsSettings();
        d->trimDiagnostics();
    }

    void SynthesisTaskManager::shutdown() {
        Q_D(SynthesisTaskManager);
        if (d->shuttingDown) {
            return;
        }
        d->shuttingDown = true;
        const auto queued = std::exchange(d->queue, {});
        for (auto task : queued) {
            d->setState(task, SynthesisTask::Canceled);
        }
        for (auto it = d->cancelFunctions.cbegin(); it != d->cancelFunctions.cend(); ++it) {
            it.value()();
        }
        d->cancelFunctions.clear();
        d->diagnosticExchanges.clear();
        d->apiClient->shutdown();
    }

}

#include "moc_SynthesisTaskManager.cpp"
