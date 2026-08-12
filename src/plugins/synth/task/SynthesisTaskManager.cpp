#include "SynthesisTaskManager.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <utility>

#include <QCryptographicHash>
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
#include <QSet>
#include <QSettings>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>

#include <CoreApi/runtimeinterface.h>

#include <synth/ServiceTypes.h>
#include <synth/SynthInterface.h>
#include <synth/SynthesisTask.h>
#include <synth/internal/ApiClient.h>
#include <synth/internal/Dtos.h>
#include <synth/private/SynthesisTask_p.h>

namespace Synth {

    Q_STATIC_LOGGING_CATEGORY(lcSynthesisTaskManager, "diffscope.synth.tasks")

    namespace {

        using namespace Internal::Api;

        QString typeName(SynthesisTaskType type) {
            switch (type) {
                case SynthesisTaskType::Pronunciation: return QStringLiteral("pronunciation");
                case SynthesisTaskType::Phoneme: return QStringLiteral("phoneme");
                case SynthesisTaskType::Duration: return QStringLiteral("duration");
                case SynthesisTaskType::Parameter: return QStringLiteral("parameter");
                case SynthesisTaskType::Audio: return QStringLiteral("audio");
            }
            return QStringLiteral("unknown");
        }

        QJsonArray doublesToJson(const QList<double> &values) {
            QJsonArray result;
            for (double value : values) {
                result.append(value);
            }
            return result;
        }

        QList<double> doublesFromJson(const QJsonValue &value) {
            QList<double> result;
            for (const auto &item : value.toArray()) {
                result.append(item.toDouble());
            }
            return result;
        }

        QJsonObject singerToJson(const SynthesisSinger &singer) {
            return {
                {QStringLiteral("id"), singer.id},
                {QStringLiteral("extra"), singer.extra},
            };
        }

        QJsonObject contextToJson(const SynthesisContext &context) {
            QJsonArray singers;
            for (const auto &singer : context.singers) {
                singers.append(singerToJson(singer));
            }
            return {
                {QStringLiteral("architecture"), context.architectureId},
                {QStringLiteral("architectureExtra"), context.architectureExtra},
                {QStringLiteral("singers"), singers},
            };
        }

        QJsonObject phonemeToJson(const SynthesisPhoneme &phoneme) {
            return {
                {QStringLiteral("token"), phoneme.token},
                {QStringLiteral("onset"), phoneme.onset},
                {QStringLiteral("language"), phoneme.language},
                {QStringLiteral("start"), phoneme.start},
            };
        }

        SynthesisPhoneme phonemeFromJson(const QJsonObject &object) {
            return {
                object.value(QStringLiteral("token")).toString(),
                object.value(QStringLiteral("onset")).toBool(),
                object.value(QStringLiteral("language")).toString(),
                object.value(QStringLiteral("start")).toDouble(),
            };
        }

        QJsonObject parameterToJson(const SynthesisParameter &parameter) {
            return {
                {QStringLiteral("values"), doublesToJson(parameter.values)},
                {QStringLiteral("sampleRate"), parameter.sampleRate},
            };
        }

        SynthesisParameter parameterFromJson(const QJsonObject &object) {
            return {
                doublesFromJson(object.value(QStringLiteral("values"))),
                object.value(QStringLiteral("sampleRate")).toDouble(100.0),
            };
        }

        QJsonObject scoreCommonToJson(const SynthesisScore &score) {
            QJsonArray notes;
            for (const auto &note : score.notes) {
                QJsonArray phonemes;
                for (const auto &phoneme : note.phonemes) {
                    phonemes.append(phonemeToJson(phoneme));
                }
                notes.append(QJsonObject{
                    {QStringLiteral("gap"), note.gap},
                    {QStringLiteral("duration"), note.duration},
                    {QStringLiteral("cent"), note.cent},
                    {QStringLiteral("pronunciation"), note.pronunciation},
                    {QStringLiteral("language"), note.language},
                    {QStringLiteral("phonemes"), phonemes},
                });
            }
            QJsonArray mix;
            for (const auto &row : score.mix) {
                mix.append(doublesToJson(row));
            }
            return {
                {QStringLiteral("pieceDuration"), score.pieceDuration},
                {QStringLiteral("notes"), notes},
                {QStringLiteral("mix"), mix},
                {QStringLiteral("mixSampleRate"), score.mixSampleRate},
            };
        }

        QJsonObject requestToJson(const SynthesisTaskRequest &request) {
            QJsonArray lyrics;
            for (const auto &note : request.lyricNotes) {
                lyrics.append(QJsonObject{
                    {QStringLiteral("lyric"), note.lyric},
                    {QStringLiteral("language"), note.language},
                });
            }
            QJsonArray pronunciations;
            for (const auto &note : request.pronunciationNotes) {
                pronunciations.append(QJsonObject{
                    {QStringLiteral("pronunciation"), note.pronunciation},
                    {QStringLiteral("language"), note.language},
                });
            }
            auto score = scoreCommonToJson(request.score);
            QJsonObject parameters;
            for (auto it = request.score.parameters.cbegin(); it != request.score.parameters.cend(); ++it) {
                parameters.insert(it.key(), parameterToJson(it.value()));
            }
            score.insert(QStringLiteral("parameters"), parameters);
            QJsonArray requested;
            for (const auto &id : request.score.requestedParameters) {
                requested.append(id);
            }
            score.insert(QStringLiteral("requestedParameters"), requested);
            return {
                {QStringLiteral("type"), typeName(request.type)},
                {QStringLiteral("context"), contextToJson(request.context)},
                {QStringLiteral("lyrics"), lyrics},
                {QStringLiteral("pronunciations"), pronunciations},
                {QStringLiteral("score"), score},
            };
        }

        QJsonObject resultToJson(const SynthesisTaskResult &result) {
            QJsonArray pronunciations;
            for (const auto &item : result.pronunciations) {
                QJsonArray candidates;
                for (const auto &candidate : item.candidates) {
                    candidates.append(candidate);
                }
                pronunciations.append(QJsonObject{
                    {QStringLiteral("pronunciation"), item.pronunciation},
                    {QStringLiteral("candidates"), candidates},
                });
            }
            QJsonArray phonemeNotes;
            for (const auto &note : result.phonemes) {
                QJsonArray phonemes;
                for (const auto &phoneme : note) {
                    phonemes.append(phonemeToJson(phoneme));
                }
                phonemeNotes.append(phonemes);
            }
            QJsonObject parameters;
            for (auto it = result.parameters.cbegin(); it != result.parameters.cend(); ++it) {
                parameters.insert(it.key(), parameterToJson(it.value()));
            }
            return {
                {QStringLiteral("pronunciations"), pronunciations},
                {QStringLiteral("phonemes"), phonemeNotes},
                {QStringLiteral("parameters"), parameters},
                {QStringLiteral("audioFilePath"), result.audioFilePath},
            };
        }

        bool resultFromJson(const QJsonObject &object, SynthesisTaskResult *result) {
            if (!result) {
                return false;
            }
            SynthesisTaskResult parsed;
            for (const auto &value : object.value(QStringLiteral("pronunciations")).toArray()) {
                const auto item = value.toObject();
                QStringList candidates;
                for (const auto &candidate : item.value(QStringLiteral("candidates")).toArray()) {
                    candidates.append(candidate.toString());
                }
                parsed.pronunciations.append({
                    item.value(QStringLiteral("pronunciation")).toString(),
                    candidates,
                });
            }
            for (const auto &value : object.value(QStringLiteral("phonemes")).toArray()) {
                QList<SynthesisPhoneme> note;
                for (const auto &phoneme : value.toArray()) {
                    note.append(phonemeFromJson(phoneme.toObject()));
                }
                parsed.phonemes.append(note);
            }
            const auto parameters = object.value(QStringLiteral("parameters")).toObject();
            for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it) {
                parsed.parameters.insert(it.key(), parameterFromJson(it.value().toObject()));
            }
            parsed.audioFilePath = object.value(QStringLiteral("audioFilePath")).toString();
            *result = std::move(parsed);
            return true;
        }

        QByteArray digest(const QJsonObject &object) {
            return QCryptographicHash::hash(QJsonDocument(object).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex();
        }

        V1::MultiSingerContext multiContext(const SynthesisContext &context) {
            V1::MultiSingerContext result;
            result.arch = context.architectureId;
            result.archExtra = context.architectureExtra;
            for (const auto &singer : context.singers) {
                result.singers.append({singer.id, singer.extra});
            }
            return result;
        }

        V1::SingleSingerContext singleContext(const SynthesisContext &context) {
            V1::SingleSingerContext result;
            result.arch = context.architectureId;
            result.archExtra = context.architectureExtra;
            if (!context.singers.isEmpty()) {
                result.singer = {context.singers.first().id, context.singers.first().extra};
            }
            return result;
        }

        QList<V1::ParameterNote> parameterNotes(const SynthesisScore &score) {
            QList<V1::ParameterNote> result;
            for (const auto &note : score.notes) {
                V1::ParameterNote converted;
                converted.position = {note.gap, note.duration};
                converted.cent = note.cent;
                converted.pronunciation = note.pronunciation;
                converted.language = note.language;
                for (const auto &phoneme : note.phonemes) {
                    converted.phonemes.append({phoneme.token, phoneme.onset, phoneme.language, phoneme.start});
                }
                result.append(converted);
            }
            return result;
        }

        V1::Mix mixToDto(const QList<QList<double>> &mix) {
            V1::Mix result;
            result.rows = mix;
            return result;
        }

        QString apiErrorText(const ApiError &error) {
            if (!error.message.isEmpty()) {
                return error.message;
            }
            if (error.httpStatusCode) {
                return SynthesisTaskManager::tr("The synthesis service returned HTTP %1.")
                    .arg(error.httpStatusCode);
            }
            return SynthesisTaskManager::tr("The synthesis service request failed.");
        }

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
                return SynthesisTaskManager::tr("The audio download failed: %1")
                    .arg(reply->errorString());
            }

            const int status = statusAttribute.toInt();
            const auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute)
                                    .toString()
                                    .trimmed();
            auto message = reason.isEmpty()
                               ? SynthesisTaskManager::tr("The audio server returned HTTP status %1.")
                                     .arg(status)
                               : SynthesisTaskManager::tr("The audio server returned HTTP status %1: %2.")
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

        class CacheStore {
        public:
            CacheStore() { reload(); }

            void reload() {
                auto settings = Core::RuntimeInterface::settings();
                settings->beginGroup(QStringLiteral("org.diffscope.synth"));
                maximumBytes = settings->value(QStringLiteral("cacheMaximumBytes"), qint64(10) * 1024 * 1024 * 1024).toLongLong();
                expiryDays = settings->value(QStringLiteral("cacheExpiryDays"), 30).toInt();
                maximumDownloadBytes = settings->value(QStringLiteral("audioDownloadMaximumBytes"), qint64(512) * 1024 * 1024).toLongLong();
                environmentTagTtlSeconds = settings->value(QStringLiteral("environmentTagTtlSeconds"), 60).toInt();
                settings->endGroup();
                QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
                if (cacheLocation.isEmpty()) {
                    cacheLocation = QDir(QDir::tempPath()).filePath(QStringLiteral("DiffScope-cache"));
                }
                root = QDir(cacheLocation).filePath(QStringLiteral("synth/v1"));
                QDir().mkpath(root);
                trim();
            }

            bool read(const QByteArray &key, SynthesisTaskResult *result) const {
                const QFileInfo info(entryPath(key));
                if (!info.exists()) {
                    return false;
                }
                if (expiryDays > 0 && info.lastModified().daysTo(QDateTime::currentDateTime()) > expiryDays) {
                    return false;
                }
                QFile file(info.absoluteFilePath());
                if (!file.open(QIODevice::ReadOnly)) {
                    return false;
                }
                QJsonParseError error;
                const auto document = QJsonDocument::fromJson(file.readAll(), &error);
                if (error.error != QJsonParseError::NoError || !document.isObject()) {
                    return false;
                }
                SynthesisTaskResult parsed;
                if (!resultFromJson(document.object().value(QStringLiteral("result")).toObject(), &parsed)) {
                    return false;
                }
                if (!parsed.audioFilePath.isEmpty() && !QFileInfo::exists(parsed.audioFilePath)) {
                    return false;
                }
                file.setFileTime(QDateTime::currentDateTime(), QFileDevice::FileModificationTime);
                if (!parsed.audioFilePath.isEmpty()) {
                    QFile audioFile(parsed.audioFilePath);
                    if (audioFile.open(QIODevice::ReadOnly)) {
                        audioFile.setFileTime(QDateTime::currentDateTime(), QFileDevice::FileModificationTime);
                    }
                }
                parsed.fromCache = true;
                *result = std::move(parsed);
                return true;
            }

            bool write(const QByteArray &key, const SynthesisTaskResult &result) {
                QSaveFile file(entryPath(key));
                if (!file.open(QIODevice::WriteOnly)) {
                    return false;
                }
                file.write(QJsonDocument(QJsonObject{
                                             {QStringLiteral("version"), 1},
                                             {QStringLiteral("result"), resultToJson(result)},
                                         })
                               .toJson(QJsonDocument::Compact));
                const bool committed = file.commit();
                if (committed) {
                    trim();
                }
                return committed;
            }

            QString audioPath(const QByteArray &key, const QString &suffix, bool persistent) const {
                const QString safeSuffix = suffix.isEmpty() ? QStringLiteral(".audio") : suffix;
                if (persistent) {
                    return QDir(root).filePath(QString::fromLatin1(key) + safeSuffix);
                }
                if (!temporaryDir.isValid()) {
                    return {};
                }
                return QDir(temporaryDir.path()).filePath(QString::fromLatin1(key) + safeSuffix);
            }

            bool writeBytes(const QString &path, const QByteArray &bytes) const {
                if (path.isEmpty()) {
                    return false;
                }
                QSaveFile file(path);
                if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) {
                    return false;
                }
                return file.commit();
            }

            qint64 size() const {
                qint64 total{};
                const auto files = QDir(root).entryInfoList(QDir::Files);
                for (const auto &file : files) {
                    total += file.size();
                }
                return total;
            }

            void clear() {
                const auto files = QDir(root).entryInfoList(QDir::Files);
                for (const auto &file : files) {
                    QFile::remove(file.absoluteFilePath());
                }
            }

            void trim() const {
                auto files = QDir(root).entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
                qint64 total{};
                const auto now = QDateTime::currentDateTime();
                for (const auto &file : files) {
                    if (expiryDays > 0 && file.lastModified().daysTo(now) > expiryDays) {
                        QFile::remove(file.absoluteFilePath());
                    } else {
                        total += file.size();
                    }
                }
                if (maximumBytes <= 0 || total <= maximumBytes) {
                    return;
                }
                files = QDir(root).entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
                for (const auto &file : files) {
                    if (total <= maximumBytes) {
                        break;
                    }
                    const auto bytes = file.size();
                    if (QFile::remove(file.absoluteFilePath())) {
                        total -= bytes;
                    }
                }
            }

            QString entryPath(const QByteArray &key) const {
                return QDir(root).filePath(QString::fromLatin1(key) + QStringLiteral(".json"));
            }

            QString root;
            qint64 maximumBytes{};
            int expiryDays{};
            qint64 maximumDownloadBytes{};
            int environmentTagTtlSeconds{};
            mutable QTemporaryDir temporaryDir;
        };

    }

    class SynthesisTaskManagerPrivate {
        Q_DECLARE_PUBLIC(SynthesisTaskManager)

    public:
        explicit SynthesisTaskManagerPrivate(SynthesisTaskManager *q)
            : q_ptr(q), apiClient(new ApiClient(q)), networkManager(new QNetworkAccessManager(q)) {
            if (auto interface = SynthInterface::instance()) {
                QObject::connect(interface, &SynthInterface::serviceInstancesChanged, q, [this] { pump(); });
                QObject::connect(interface, &SynthInterface::serviceInstanceDetailsChanged, q, [this](const QUuid &) { pump(); });
            }
            QTimer::singleShot(0, q, [this] { pump(); });
        }

        struct EnvironmentTagEntry {
            QString tag;
            QDateTime expiresAt;
        };

        SynthesisTaskManager *q_ptr{};
        ApiClient *apiClient{};
        QNetworkAccessManager *networkManager{};
        CacheStore cache;
        QList<SynthesisTask *> tasks;
        QList<SynthesisTask *> queue;
        QHash<QUuid, int> activeByService;
        QHash<QString, int> activeByServiceAndType;
        QHash<SynthesisTask *, std::function<void()>> cancelFunctions;
        QHash<QByteArray, EnvironmentTagEntry> environmentTags;
        bool shuttingDown{};

        static QString counterKey(const QUuid &serviceId, SynthesisTaskType type) {
            return serviceId.toString(QUuid::WithoutBraces) + u':' + typeName(type);
        }

        std::optional<ServiceInstanceConfiguration> resolveService(const SynthesisContext &context) const {
            auto interface = SynthInterface::instance();
            if (!interface || context.architectureId.isEmpty() || context.singers.isEmpty()) {
                return std::nullopt;
            }
            for (const auto &configuration : interface->serviceInstances()) {
                if (!configuration.isEnabled()) {
                    continue;
                }
                const auto details = interface->serviceInstanceDetails(configuration.id());
                if (details.healthStatus() != ServiceInstanceDetails::Healthy) {
                    continue;
                }
                bool hasArchitecture = false;
                for (const auto &architecture : details.metadata().architectures()) {
                    if (architecture.id() == context.architectureId) {
                        hasArchitecture = true;
                        break;
                    }
                }
                if (!hasArchitecture) {
                    continue;
                }
                bool hasEverySinger = true;
                const auto availableSingers = details.metadata().singers();
                for (const auto &requested : context.singers) {
                    const auto found = std::ranges::find_if(availableSingers, [&](const SingerMetadata &singer) {
                        return singer.id() == requested.id &&
                               singer.architectureId() == context.architectureId;
                    });
                    if (found == availableSingers.cend()) {
                        hasEverySinger = false;
                        break;
                    }
                }
                if (hasEverySinger) {
                    return configuration;
                }
            }
            return std::nullopt;
        }

        bool serviceResolutionPending() const {
            auto interface = SynthInterface::instance();
            if (!interface)
                return false;
            for (const auto &configuration : interface->serviceInstances()) {
                if (!configuration.isEnabled())
                    continue;
                const auto status = interface->serviceInstanceDetails(configuration.id()).healthStatus();
                if (status == ServiceInstanceDetails::Unknown || status == ServiceInstanceDetails::Checking)
                    return true;
            }
            return false;
        }

        ArchitectureMetadata architectureMetadata(const ServiceInstanceConfiguration &service, const QString &architectureId) const {
            const auto details = SynthInterface::instance()->serviceInstanceDetails(service.id());
            for (const auto &architecture : details.metadata().architectures()) {
                if (architecture.id() == architectureId) {
                    return architecture;
                }
            }
            return {};
        }

        QJsonObject metadataJson(const ServiceInstanceConfiguration &service, const SynthesisContext &context) const {
            const auto details = SynthInterface::instance()->serviceInstanceDetails(service.id());
            QJsonArray singers;
            for (const auto &requested : context.singers) {
                for (const auto &singer : details.metadata().singers()) {
                    if (singer.id() == requested.id &&
                        singer.architectureId() == context.architectureId) {
                        singers.append(singer.toJson());
                        break;
                    }
                }
            }
            return {
                {QStringLiteral("architecture"), architectureMetadata(service, context.architectureId).toJson()},
                {QStringLiteral("singers"), singers},
            };
        }

        QJsonObject cacheEnvelope(const ServiceInstanceConfiguration &service, const SynthesisTaskRequest &request, const QString &environmentTag) const {
            const auto details = SynthInterface::instance()->serviceInstanceDetails(service.id());
            return {
                {QStringLiteral("schema"), 2},
                {QStringLiteral("serviceId"), service.id().toString(QUuid::WithoutBraces)},
                {QStringLiteral("serviceUrl"), service.baseUrl().toString(QUrl::FullyEncoded)},
                {QStringLiteral("apiVersion"), details.selectedApiVersion()},
                {QStringLiteral("environmentTag"), environmentTag},
                {QStringLiteral("metadata"), metadataJson(service, request.context)},
                {QStringLiteral("request"), requestToJson(request)},
            };
        }

        QByteArray taskCacheKey(const ServiceInstanceConfiguration &service, const SynthesisTaskRequest &request, const QString &environmentTag) const {
            return digest(cacheEnvelope(service, request, environmentTag));
        }

        QByteArray parameterCacheKey(const ServiceInstanceConfiguration &service, const SynthesisTaskRequest &request, const QString &environmentTag, const QString &target, const QMap<QString, SynthesisParameter> &parameters, const QSet<QString> &unavailableParameters = {}) const {
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
            return digest(envelope);
        }

        void setService(SynthesisTask *task, const ServiceInstanceConfiguration &service) {
            auto d = SynthesisTaskPrivate::get(task);
            if (d->serviceId == service.id() && d->serviceName == service.name()) {
                return;
            }
            d->serviceId = service.id();
            d->serviceName = service.name();
            Q_EMIT task->serviceChanged();
            Q_EMIT q_ptr->taskChanged(task);
        }

        void setState(SynthesisTask *task, SynthesisTask::State state, const QString &error = {}) {
            auto d = SynthesisTaskPrivate::get(task);
            if (d->state == state && d->errorMessage == error) {
                return;
            }
            d->state = state;
            d->errorMessage = error;
            if (state == SynthesisTask::Running) {
                d->startedAt = QDateTime::currentDateTimeUtc();
            }
            if (state == SynthesisTask::Succeeded || state == SynthesisTask::Failed ||
                state == SynthesisTask::Canceled) {
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

        bool canStart(const ServiceInstanceConfiguration &service, SynthesisTaskType type) const {
            return activeByService.value(service.id()) < service.globalConcurrency() &&
                   activeByServiceAndType.value(counterKey(service.id(), type)) < service.taskConcurrency();
        }

        void pump() {
            if (shuttingDown) {
                return;
            }
            std::stable_sort(queue.begin(), queue.end(), [](SynthesisTask *left, SynthesisTask *right) {
                return left->priority() > right->priority();
            });
            for (qsizetype index = 0; index < queue.size();) {
                SynthesisTask *task = queue.at(index);
                const auto service = resolveService(task->request().context);
                if (!service) {
                    if (serviceResolutionPending()) {
                        ++index;
                        continue;
                    }
                    queue.removeAt(index);
                    setState(task, SynthesisTask::Failed, SynthesisTaskManager::tr("No healthy synthesis service provides the selected architecture and every singer."));
                    continue;
                }
                setService(task, *service);
                if (!canStart(*service, task->type())) {
                    ++index;
                    continue;
                }
                queue.removeAt(index);
                start(task, *service);
            }
        }

        void start(SynthesisTask *task, const ServiceInstanceConfiguration &service) {
            setState(task, SynthesisTask::Running);
            ++activeByService[service.id()];
            ++activeByServiceAndType[counterKey(service.id(), task->type())];
            Q_EMIT q_ptr->serviceTaskCountsChanged(service.id());
            obtainEnvironmentTag(task, service);
        }

        void releaseSlot(SynthesisTask *task) {
            const auto serviceId = task->serviceInstanceId();
            activeByService[serviceId] = std::max(0, activeByService.value(serviceId) - 1);
            const auto key = counterKey(serviceId, task->type());
            activeByServiceAndType[key] = std::max(0, activeByServiceAndType.value(key) - 1);
            cancelFunctions.remove(task);
            Q_EMIT q_ptr->serviceTaskCountsChanged(serviceId);
            QTimer::singleShot(0, q_ptr, [this] { pump(); });
        }

        void complete(SynthesisTask *task, SynthesisTaskResult result) {
            if (task->state() != SynthesisTask::Running) {
                return;
            }
            SynthesisTaskPrivate::get(task)->result = std::move(result);
            setState(task, SynthesisTask::Succeeded);
            releaseSlot(task);
        }

        void fail(SynthesisTask *task, const QString &message) {
            if (task->state() != SynthesisTask::Running) {
                return;
            }
            setState(task, SynthesisTask::Failed, message);
            releaseSlot(task);
        }

        void finishCanceled(SynthesisTask *task) {
            if (task->state() != SynthesisTask::Running) {
                return;
            }
            setState(task, SynthesisTask::Canceled);
            releaseSlot(task);
        }

        template <typename T, typename Callback>
        void watch(SynthesisTask *task, QFuture<ApiResult<T>> future, Callback callback) {
            auto watcher = new QFutureWatcher<ApiResult<T>>(q_ptr);
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
                callback(std::move(result));
            });
            watcher->setFuture(future);
        }

        QByteArray environmentKey(const ServiceInstanceConfiguration &service, const SynthesisContext &context) const {
            return digest({
                {QStringLiteral("service"), service.id().toString(QUuid::WithoutBraces)},
                {QStringLiteral("context"), contextToJson(context)},
            });
        }

        void obtainEnvironmentTag(SynthesisTask *task, const ServiceInstanceConfiguration &service) {
            const auto key = environmentKey(service, task->request().context);
            const auto cached = environmentTags.constFind(key);
            if (cached != environmentTags.cend() && cached->expiresAt > QDateTime::currentDateTimeUtc()) {
                execute(task, service, cached->tag);
                return;
            }
            V1::EnvTagRequest request;
            request.context = multiContext(task->request().context);
            watch(task, apiClient->createEnvironmentTag(service, request), [this, task, service, key](ApiResult<V1::EnvTagResponse> result) {
                if (!result) {
                    fail(task, apiErrorText(result.error()));
                    return;
                }
                const auto tag = result.value().envTag;
                environmentTags.insert(key, {
                                                tag,
                                                QDateTime::currentDateTimeUtc().addSecs(std::max(1, cache.environmentTagTtlSeconds)),
                                            });
                execute(task, service, tag);
            });
        }

        void execute(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag) {
            if (task->state() != SynthesisTask::Running) {
                return;
            }
            if (task->type() == SynthesisTaskType::Parameter) {
                executeParameter(task, service, environmentTag);
                return;
            }
            const auto key = taskCacheKey(service, task->request(), environmentTag);
            SynthesisTaskResult cached;
            if (task->options().readCache && cache.read(key, &cached)) {
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

        void maybeWriteAndComplete(SynthesisTask *task, const QByteArray &key, SynthesisTaskResult result) {
            if (task->options().writeCache) {
                cache.write(key, result);
            }
            complete(task, std::move(result));
        }

        void executePronunciation(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &, const QByteArray &key) {
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

        void executePhoneme(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &, const QByteArray &key) {
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

        void executeDuration(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &, const QByteArray &key) {
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

        void executeParameter(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &environmentTag) {
            auto requestModel = task->request();
            SynthesisTaskResult combined;
            QStringList unresolved;
            QStringList pending = requestModel.score.requestedParameters;
            QSet<QString> unavailableParameters(pending.cbegin(), pending.cend());
            bool progressed = true;
            while (progressed && !pending.isEmpty()) {
                progressed = false;
                QStringList next;
                for (const auto &target : pending) {
                    const auto key = parameterCacheKey(service, requestModel, environmentTag, target, requestModel.score.parameters, unavailableParameters);
                    SynthesisTaskResult cached;
                    if (!key.isEmpty() && task->options().readCache && cache.read(key, &cached) &&
                        cached.parameters.contains(target)) {
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
            unresolved = pending;
            if (unresolved.isEmpty()) {
                complete(task, std::move(combined));
                return;
            }
            requestModel.score.requestedParameters = unresolved;
            for (const auto &target : unresolved) {
                if (!requestModel.score.parameters.contains(target)) {
                    requestModel.score.parameters.insert(target, {
                                                                     {},
                                                                     100.0,
                                                                 });
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
                if (unresolved.contains(it.key())) {
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
                for (auto it = response.value().output.parameters.cbegin();
                     it != response.value().output.parameters.cend(); ++it) {
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
                            cache.write(key, node);
                        }
                    }
                }
                complete(task, std::move(combined));
            });
        }

        void executeAudio(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QString &, const QByteArray &key) {
            V1::AudioRequest request;
            request.context = multiContext(task->request().context);
            request.input.pieceDuration = task->request().score.pieceDuration;
            request.input.notes = parameterNotes(task->request().score);
            request.input.mix = mixToDto(task->request().score.mix);
            request.input.mixSampleRate = task->request().score.mixSampleRate;
            for (auto it = task->request().score.parameters.cbegin();
                 it != task->request().score.parameters.cend(); ++it) {
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

        QString suffixForMime(const QString &mimeName) const {
            const auto suffixes = QMimeDatabase().mimeTypeForName(mimeName).suffixes();
            return suffixes.isEmpty() ? QStringLiteral(".audio") : u'.' + suffixes.first();
        }

        void finishAudioBytes(SynthesisTask *task, const QByteArray &key, const QByteArray &bytes, const QString &suffix) {
            if (bytes.isEmpty() || bytes.size() > cache.maximumDownloadBytes) {
                fail(task, SynthesisTaskManager::tr("The synthesized audio response is empty or exceeds the configured size limit."));
                return;
            }
            const auto path = cache.audioPath(key, suffix, task->options().writeCache);
            if (!cache.writeBytes(path, bytes)) {
                fail(task, SynthesisTaskManager::tr("Could not store synthesized audio locally."));
                return;
            }
            SynthesisTaskResult result;
            result.audioFilePath = path;
            maybeWriteAndComplete(task, key, std::move(result));
        }

        void materializeAudio(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QByteArray &key, const QString &location) {
            if (location.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive)) {
                const auto comma = location.indexOf(u',');
                if (comma <= 5) {
                    fail(task, SynthesisTaskManager::tr("The synthesis service returned an invalid audio data URL."));
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
            if (!url.isValid() || (url.scheme() != QStringLiteral("http") &&
                                   url.scheme() != QStringLiteral("https"))) {
                fail(task, SynthesisTaskManager::tr("The synthesis service returned an unsupported audio URL."));
                return;
            }
            downloadAudio(task, service, key, url, 0);
        }

        static int effectivePort(const QUrl &url) {
            return url.port(url.scheme() == QStringLiteral("https") ? 443 : 80);
        }

        static bool sameOrigin(const QUrl &left, const QUrl &right) {
            return left.scheme().compare(right.scheme(), Qt::CaseInsensitive) == 0 &&
                   left.host().compare(right.host(), Qt::CaseInsensitive) == 0 &&
                   effectivePort(left) == effectivePort(right);
        }

        void downloadAudio(SynthesisTask *task, const ServiceInstanceConfiguration &service, const QByteArray &key, const QUrl &url, int redirectCount) {
            if (redirectCount > 5) {
                fail(task, SynthesisTaskManager::tr("The synthesized audio URL redirected too many times."));
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
                if (received > cache.maximumDownloadBytes || total > cache.maximumDownloadBytes) {
                    cancelFunctions.remove(task);
                    reply->setProperty("synthDownloadTooLarge", true);
                    reply->abort();
                }
            });
            QObject::connect(reply, &QNetworkReply::finished, q_ptr, [this, task, service, key, redirectCount, reply] {
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
                                             ? SynthesisTaskManager::tr("The synthesized audio exceeds the configured download size limit.")
                                             : audioDownloadErrorText(reply, body);
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
    };

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
        if (const auto service = d->resolveService(request.context)) {
            d->setService(task, *service);
        }
        Q_EMIT taskAdded(task);
        Q_EMIT taskCountsChanged();
        if (!task->serviceInstanceId().isNull()) {
            Q_EMIT serviceTaskCountsChanged(task->serviceInstanceId());
        }
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
            d->setState(task, SynthesisTask::Canceled);
            return true;
        }
        const auto cancel = d->cancelFunctions.value(task);
        if (cancel) {
            d->setState(task, SynthesisTask::Canceled);
            d->releaseSlot(task);
            cancel();
        }
        return true;
    }

    void SynthesisTaskManager::clearFinishedTasks() {
        Q_D(SynthesisTaskManager);
        for (qsizetype index = d->tasks.size() - 1; index >= 0; --index) {
            auto task = d->tasks.at(index);
            if (!task->isFinished()) {
                continue;
            }
            d->tasks.removeAt(index);
            Q_EMIT taskRemoved(task);
            task->deleteLater();
        }
    }

    void SynthesisTaskManager::clearCache() {
        Q_D(SynthesisTaskManager);
        d->cache.clear();
    }

    void SynthesisTaskManager::reloadSettings() {
        Q_D(SynthesisTaskManager);
        d->cache.reload();
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
        d->apiClient->shutdown();
    }

}

#include "moc_SynthesisTaskManager.cpp"
