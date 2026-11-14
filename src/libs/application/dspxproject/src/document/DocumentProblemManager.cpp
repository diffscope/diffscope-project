// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DocumentProblemManager.h"
#include "DocumentProblemManager_p.h"

#include <algorithm>
#include <utility>

#include <QMetaObject>
#include <QPointer>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/AudioDSP.h>
#include <dspxmodelORM/AudioDSPList.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/KeySignatureSequence.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/LabelSequence.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/TimeSignature.h>
#include <dspxmodelORM/TimeSignatureSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <dspxproject/Document.h>

namespace DspxProject {

    dspx::EntityObject *DocumentProblemManagerPrivate::parentEntity(dspx::EntityObject *entity, QMetaObject::Connection *connection) {
        const auto watch = [this, connection](auto *item, auto signal) {
            if (connection)
                *connection = QObject::connect(item, signal, q_ptr, [this, handle = item->handle()] { markTopologyDirty(handle); });
        };
        if (auto *track = qobject_cast<dspx::Track *>(entity)) {
            watch(track, &dspx::Track::trackListChanged);
            auto *list = track->trackList();
            return list ? list->model() : nullptr;
        }
        if (auto *clip = qobject_cast<dspx::Clip *>(entity)) {
            watch(clip, &dspx::Clip::clipSequenceChanged);
            auto *sequence = clip->clipSequence();
            return sequence ? sequence->track() : nullptr;
        }
        if (auto *note = qobject_cast<dspx::Note *>(entity)) {
            watch(note, &dspx::Note::noteSequenceChanged);
            auto *sequence = note->noteSequence();
            return sequence ? sequence->singingClip() : nullptr;
        }
        if (auto *phoneme = qobject_cast<dspx::Phoneme *>(entity)) {
            watch(phoneme, &dspx::Phoneme::phonemeSequenceChanged);
            auto *sequence = phoneme->phonemeSequence();
            return sequence ? sequence->note() : nullptr;
        }
        if (auto *parameter = qobject_cast<dspx::Parameter *>(entity)) {
            watch(parameter, &dspx::Parameter::parameterMapChanged);
            auto *map = parameter->parameterMap();
            return map ? map->singingClip() : nullptr;
        }
        if (auto *anchor = qobject_cast<dspx::AnchorNode *>(entity)) {
            watch(anchor, &dspx::AnchorNode::anchorNodeSequenceChanged);
            auto *sequence = anchor->anchorNodeSequence();
            return sequence ? sequence->parameter() : nullptr;
        }
        if (auto *sources = qobject_cast<dspx::Sources *>(entity)) {
            watch(sources, &dspx::Sources::singingClipChanged);
            return sources->singingClip();
        }
        if (auto *singer = qobject_cast<dspx::Singer *>(entity)) {
            watch(singer, &dspx::Singer::singerListChanged);
            auto *list = singer->singerList();
            if (!list)
                return nullptr;
            if (auto *sources = list->sources())
                return sources;
            return list->mixedSinger();
        }
        if (auto *anchor = qobject_cast<dspx::DynamicMixingAnchor *>(entity)) {
            watch(anchor, &dspx::DynamicMixingAnchor::dynamicMixingAnchorSequenceChanged);
            auto *sequence = anchor->dynamicMixingAnchorSequence();
            return sequence ? sequence->sources() : nullptr;
        }
        if (auto *audioDSP = qobject_cast<dspx::AudioDSP *>(entity)) {
            watch(audioDSP, &dspx::AudioDSP::audioDSPListChanged);
            auto *list = audioDSP->audioDSPList();
            if (list == model->audioDSPs())
                return model;
            return list ? list->track() : nullptr;
        }
        if (auto *label = qobject_cast<dspx::Label *>(entity)) {
            watch(label, &dspx::Label::labelSequenceChanged);
            auto *sequence = label->labelSequence();
            return sequence ? sequence->model() : nullptr;
        }
        if (auto *tempo = qobject_cast<dspx::Tempo *>(entity)) {
            watch(tempo, &dspx::Tempo::tempoSequenceChanged);
            auto *sequence = tempo->tempoSequence();
            return sequence ? sequence->model() : nullptr;
        }
        if (auto *signature = qobject_cast<dspx::TimeSignature *>(entity)) {
            watch(signature, &dspx::TimeSignature::timeSignatureSequenceChanged);
            auto *sequence = signature->timeSignatureSequence();
            return sequence ? sequence->model() : nullptr;
        }
        if (auto *signature = qobject_cast<dspx::KeySignature *>(entity)) {
            watch(signature, &dspx::KeySignature::keySignatureSequenceChanged);
            auto *sequence = signature->keySignatureSequence();
            return sequence ? sequence->model() : nullptr;
        }
        return nullptr;
    }

    QList<dspx::EntityObject *> DocumentProblemManagerPrivate::entityPath(dspx::EntityObject *entity) {
        QList<dspx::EntityObject *> path;
        while (isLive(entity) && !path.contains(entity)) {
            path.append(entity);
            if (entity == model)
                break;
            entity = parentEntity(entity);
        }
        return path;
    }

    bool DocumentProblemManagerPrivate::isLive(const dspx::EntityObject *entity) const {
        return entity && entity->model() == model && model->find(*entity->metaObject(), entity->handle()) == entity;
    }

    bool DocumentProblemManagerPrivate::isAttached(const QList<dspx::Handle> &path) const {
        return !path.isEmpty() && path.back() == model->handle();
    }

    void DocumentProblemManagerPrivate::updatePath(dspx::Handle handle, Target &target, const QList<dspx::EntityObject *> &entities) {
        QList<dspx::Handle> path;
        path.reserve(entities.size());
        bool sameObjects = true;
        for (auto *entity : entities) {
            path.append(entity->handle());
            const auto it = watches.constFind(entity->handle());
            sameObjects = sameObjects && it != watches.cend() && it->entity == entity;
        }
        if (sameObjects && target.path == path)
            return;

        if (isAttached(target.path))
            invalidate(target.path);
        for (const auto ancestor : target.path) {
            auto it = watches.find(ancestor);
            it->targets.remove(handle);
            if (it->targets.isEmpty()) {
                QObject::disconnect(it->parentChanged);
                QObject::disconnect(it->destroyed);
                watches.erase(it);
            }
        }
        for (auto *entity : entities) {
            auto &watch = watches[entity->handle()];
            if (watch.entity != entity) {
                QObject::disconnect(watch.parentChanged);
                QObject::disconnect(watch.destroyed);
                watch.entity = entity;
                parentEntity(entity, &watch.parentChanged);
                watch.destroyed = QObject::connect(entity, &QObject::destroyed, q_ptr, [this, handle = entity->handle()] { markTopologyDirty(handle); });
            }
            watch.targets.insert(handle);
        }
        target.path = std::move(path);
        if (isAttached(target.path))
            invalidate(target.path);
    }

    void DocumentProblemManagerPrivate::markTopologyDirty(dspx::Handle handle) {
        dirtyWatches.insert(handle);
        queueNotification();
    }

    void DocumentProblemManagerPrivate::refreshTopology() {
        QSet<dspx::Handle> dirtyTargets;
        for (const auto handle : std::exchange(dirtyWatches, {})) {
            const auto watch = watches.constFind(handle);
            if (watch != watches.cend()) {
                for (const auto target : watch->targets)
                    dirtyTargets.insert(target);
            }
        }
        QList<DocumentProblemId> removed;
        for (const auto handle : std::as_const(dirtyTargets)) {
            auto it = targets.find(handle);
            const auto path = entityPath(it->entity);
            if (path.isEmpty())
                removed.append(it->ids.values());
            else
                updatePath(it.key(), it.value(), path);
        }
        for (const auto id : removed)
            removeProblem(id);
    }

    void DocumentProblemManagerPrivate::queueNotification() {
        Q_Q(DocumentProblemManager);
        if (std::exchange(notificationQueued, true))
            return;

        QMetaObject::invokeMethod(q, [this] {
            Q_Q(DocumentProblemManager);
            refreshTopology();
            notificationQueued = false;
            const auto entities = std::exchange(affectedEntities, QSet<dspx::Handle>{}).values();
            const bool notifyOperations = std::exchange(operationsDirty, false);
            const QPointer<DocumentProblemManager> manager(q);
            if (!entities.isEmpty())
                Q_EMIT q->problemsChanged(entities);
            if (manager && notifyOperations)
                Q_EMIT q->operationsChanged();
        }, Qt::QueuedConnection);
    }

    void DocumentProblemManagerPrivate::invalidate(const QList<dspx::Handle> &path) {
        for (const auto handle : path)
            affectedEntities.insert(handle);
        queueNotification();
    }

    void DocumentProblemManagerPrivate::addProblem(QObject *owner, const DocumentProblemReport &report, const QList<dspx::EntityObject *> &path) {
        const auto id = static_cast<DocumentProblemId>(nextProblemId++);
        const auto handle = report.info.entity->handle();
        auto &target = targets[handle];
        target.entity = report.info.entity;
        const bool newTarget = target.ids.isEmpty();
        if (newTarget)
            updatePath(handle, target, path);
        target.ids.insert(id);
        records.insert(id, Record{DocumentProblem{id, report.info}, owner, report.key, handle});

        auto &batch = batches[owner];
        if (batch.ids.isEmpty()) {
            batch.destroyed = QObject::connect(owner, &QObject::destroyed, q_ptr, [this, owner] {
                const auto ids = batches.value(owner).ids.values();
                for (const auto id : ids)
                    removeProblem(id);
            });
        }
        batch.ids.insert(report.key, id);
        if (!newTarget)
            invalidate(target.path);
    }

    void DocumentProblemManagerPrivate::removeProblem(DocumentProblemId id) {
        auto record = records.find(id);
        auto target = targets.find(record->target);
        target->ids.remove(id);
        if (target->ids.isEmpty()) {
            updatePath(record->target, target.value(), {});
            targets.erase(target);
        } else if (isAttached(target->path)) {
            invalidate(target->path);
        }
        auto batch = batches.find(record->owner);
        batch->ids.remove(record->key);
        if (batch->ids.isEmpty()) {
            QObject::disconnect(batch->destroyed);
            batches.erase(batch);
        }
        records.erase(record);
    }

    DocumentProblemManager::DocumentProblemManager(Document *document)
        : QObject(document), d_ptr(new DocumentProblemManagerPrivate) {
        Q_D(DocumentProblemManager);
        d->q_ptr = this;
        d->model = document->model();
    }

    DocumentProblemManager::~DocumentProblemManager() {
        QObject::disconnect(this, nullptr, this, nullptr);
    }

    bool DocumentProblemManager::replaceProblems(QObject *owner, const QList<DocumentProblemReport> &reports) {
        Q_D(DocumentProblemManager);
        return d->updateProblems(owner, reports, {}, true);
    }

    bool DocumentProblemManager::updateProblems(QObject *owner, const QList<DocumentProblemReport> &reports, const QStringList &removedKeys) {
        Q_D(DocumentProblemManager);
        return d->updateProblems(owner, reports, removedKeys, false);
    }

    bool DocumentProblemManagerPrivate::updateProblems(QObject *owner, const QList<DocumentProblemReport> &reports, QStringList removedKeys, bool replace) {
        if (!owner || owner->thread() != q_ptr->thread())
            return false;
        refreshTopology();

        QSet<QString> keys;
        QHash<dspx::EntityObject *, QList<dspx::EntityObject *>> paths;
        for (const auto &report : reports) {
            const auto &info = report.info;
            if (report.key.isEmpty() || keys.contains(report.key) || info.type.isEmpty() || info.message.isEmpty())
                return false;
            switch (info.severity) {
                case DocumentProblemSeverity::Error:
                case DocumentProblemSeverity::Warning:
                case DocumentProblemSeverity::Notice:
                    break;
                default:
                    return false;
            }
            auto *entity = info.entity.data();
            if (!paths.contains(entity)) {
                auto path = entityPath(entity);
                if (path.isEmpty() || path.back() != model)
                    return false;
                paths.insert(entity, std::move(path));
            }
            keys.insert(report.key);
        }

        for (const auto &path : std::as_const(paths)) {
            for (auto *entity : path) {
                const auto watch = watches.constFind(entity->handle());
                if (watch != watches.cend() && watch->entity != entity)
                    dirtyWatches.insert(entity->handle());
            }
        }
        refreshTopology();

        if (replace) {
            const auto batch = batches.constFind(owner);
            if (batch != batches.cend()) {
                for (auto it = batch->ids.cbegin(); it != batch->ids.cend(); ++it) {
                    if (!keys.contains(it.key()))
                        removedKeys.append(it.key());
                }
            }
        }
        const auto problemId = [this, owner](const QString &key) {
            const auto batch = batches.constFind(owner);
            return batch == batches.cend() ? DocumentProblemId::Invalid : batch->ids.value(key);
        };
        for (const auto &key : std::as_const(removedKeys)) {
            if (const auto id = problemId(key); !keys.contains(key) && id != DocumentProblemId::Invalid)
                removeProblem(id);
        }
        for (const auto &report : reports) {
            const auto &info = report.info;
            const auto id = problemId(report.key);
            if (id != DocumentProblemId::Invalid) {
                auto &record = records.find(id).value();
                auto &oldInfo = record.problem.info;
                if (oldInfo.entity == info.entity && oldInfo.type == info.type) {
                    if (oldInfo.severity != info.severity || oldInfo.message != info.message ||
                        oldInfo.data.metaType() != info.data.metaType() || oldInfo.data != info.data) {
                        oldInfo = info;
                        invalidate(targets.constFind(record.target)->path);
                    }
                    continue;
                }
                removeProblem(id);
            }
            addProblem(owner, report, paths.constFind(info.entity.data()).value());
        }
        return true;
    }

    QList<DocumentProblem> DocumentProblemManager::problems(const dspx::EntityObject *entity, const DocumentProblemFilter &filter) const {
        Q_D(const DocumentProblemManager);
        d_ptr->refreshTopology();
        if (!d->isLive(entity))
            return {};
        const auto watch = d->watches.constFind(entity->handle());
        if (watch == d->watches.cend())
            return {};

        QList<DocumentProblem> result;
        for (const auto handle : watch->targets) {
            if (filter.scope == DocumentProblemScope::Direct && handle != entity->handle())
                continue;
            const auto target = d->targets.constFind(handle);
            if (!d->isAttached(target->path))
                continue;
            for (const auto id : target->ids) {
                const auto &problem = d->records.constFind(id)->problem;
                const auto &info = problem.info;
                if (filter.severities.testFlag(info.severity) && (filter.types.isEmpty() || filter.types.contains(info.type)))
                    result.append(problem);
            }
        }
        std::ranges::sort(result, {}, &DocumentProblem::id);
        return result;
    }

    std::optional<DocumentProblem> DocumentProblemManager::problem(DocumentProblemId id) const {
        Q_D(const DocumentProblemManager);
        d_ptr->refreshTopology();
        const auto it = d->records.constFind(id);
        if (it == d->records.cend() || !d->isAttached(d->targets.constFind(it->target)->path))
            return std::nullopt;
        return it->problem;
    }

    QList<dspx::EntityObject *> DocumentProblemManager::entityPath(DocumentProblemId id) const {
        const auto current = problem(id);
        return current ? d_ptr->entityPath(current->info.entity) : QList<dspx::EntityObject *>{};
    }

    bool DocumentProblemManagerPrivate::Operation::appliesTo(const DocumentProblem &problem) const {
        return context && (type.isEmpty() || type == problem.info.type) && (!predicate || predicate(problem));
    }

    DocumentProblemOperationId DocumentProblemManagerPrivate::registerOperation(QObject *context, const QString &type, const QString &text,
                                                                                DocumentProblemManager::Handler handler,
                                                                                DocumentProblemManager::Predicate predicate) {
        Q_Q(DocumentProblemManager);
        if (!context || context->thread() != q->thread() || !handler)
            return DocumentProblemOperationId::Invalid;
        const auto id = static_cast<DocumentProblemOperationId>(nextOperationId++);
        const auto destroyed = QObject::connect(context, &QObject::destroyed, q, [q, id] { q->unregisterOperation(id); });
        operations.insert(id, QSharedPointer<Operation>::create(Operation{context, type, text, std::move(handler), std::move(predicate), destroyed}));
        operationsDirty = true;
        queueNotification();
        return id;
    }

    DocumentProblemOperationId DocumentProblemManager::registerNavigationHandler(QObject *context, Handler handler, Predicate applies) {
        Q_D(DocumentProblemManager);
        return d->registerOperation(context, {}, {}, std::move(handler), std::move(applies));
    }

    DocumentProblemOperationId DocumentProblemManager::registerAction(QObject *context, const QString &type, const QString &text,
                                                                      Handler handler, Predicate applies) {
        Q_D(DocumentProblemManager);
        if (type.isEmpty() || text.isEmpty())
            return DocumentProblemOperationId::Invalid;
        return d->registerOperation(context, type, text, std::move(handler), std::move(applies));
    }

    bool DocumentProblemManager::unregisterOperation(DocumentProblemOperationId operationId) {
        Q_D(DocumentProblemManager);
        const auto it = d->operations.find(operationId);
        if (it == d->operations.end())
            return false;
        QObject::disconnect((*it)->destroyed);
        d->operations.erase(it);
        d->operationsDirty = true;
        d->queueNotification();
        return true;
    }

    bool DocumentProblemManager::canNavigate(DocumentProblemId id) const {
        Q_D(const DocumentProblemManager);
        const auto current = problem(id);
        return current && std::ranges::any_of(d->operations, [&current](const auto &operation) {
            return operation->type.isEmpty() && operation->appliesTo(*current);
        });
    }

    bool DocumentProblemManager::navigate(DocumentProblemId id) {
        Q_D(DocumentProblemManager);
        const QPointer<DocumentProblemManager> manager(this);
        const auto ids = d->operations.keys();
        bool handled = false;
        for (const auto operationId : ids) {
            const auto current = problem(id);
            if (!current)
                break;
            const auto operation = d->operations.value(operationId);
            if (!operation || !operation->type.isEmpty() || !operation->appliesTo(*current))
                continue;
            if (operation->handler(*current))
                handled = true;
            if (!manager)
                break;
        }
        return handled;
    }

    QList<DocumentProblemAction> DocumentProblemManager::actions(DocumentProblemId id) const {
        Q_D(const DocumentProblemManager);
        const auto current = problem(id);
        if (!current)
            return {};
        QList<DocumentProblemAction> result;
        for (auto it = d->operations.cbegin(); it != d->operations.cend(); ++it) {
            const auto &operation = it.value();
            if (!operation->type.isEmpty() && operation->appliesTo(*current))
                result.append(DocumentProblemAction{it.key(), operation->text});
        }
        return result;
    }

    bool DocumentProblemManager::triggerAction(DocumentProblemId id, DocumentProblemOperationId actionId) {
        Q_D(const DocumentProblemManager);
        const auto current = problem(id);
        if (!current)
            return false;
        const auto operation = d->operations.value(actionId);
        return operation && !operation->type.isEmpty() && operation->appliesTo(*current) && operation->handler(*current);
    }

}

#include "moc_DocumentProblemManager.cpp"
