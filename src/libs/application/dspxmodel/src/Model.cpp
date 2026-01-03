#include "Model.h"
#include "Model_p.h"

#include <QVariant>

#include <opendspx/model.h>

#include <dspxmodel/AnchorNode.h>
#include <dspxmodel/AudioClip.h>
#include <dspxmodel/Global.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Master.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/Param.h>
#include <dspxmodel/ParamCurveAnchor.h>
#include <dspxmodel/ParamCurveFree.h>
#include <dspxmodel/Phoneme.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Source.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/TimeSignature.h>
#include <dspxmodel/TimeSignatureSequence.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/WorkspaceInfo.h>
#include <dspxmodel/private/EntityObject_p.h>

namespace dspx {

    void ModelPrivate::handleEntityDestroyed(Handle handle) {
        auto object = mapToObject(handle);
        if (object) {
            object->deleteLater();
            object->d_func()->handle = {};
            objectMap.remove(handle);
            handleMap.remove(object);
        }
    }

    void ModelPrivate::init() {
        Q_Q(Model);
        auto handle = strategy->createEntity(ModelStrategy::EI_Global);
        q->EntityObject::d_func()->model = q;
        q->EntityObject::d_func()->handle = handle;
        objectMap.insert(handle, q);
        handleMap.insert(q, handle);

        name = strategy->getEntityProperty(handle, ModelStrategy::P_Name).toString();
        author = strategy->getEntityProperty(handle, ModelStrategy::P_Author).toString();
        centShift = strategy->getEntityProperty(handle, ModelStrategy::P_CentShift).toInt();
        editorId = strategy->getEntityProperty(handle, ModelStrategy::P_EditorId).toString();
        editorName = strategy->getEntityProperty(handle, ModelStrategy::P_EditorName).toString();

        global = new Global(q);
        master = new Master(q);
        timeline = new Timeline(q);

        labels = new LabelSequence(strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Labels), q);
        tempos = new TempoSequence(strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Tempos), q);
        timeSignatures = new TimeSignatureSequence(strategy->getAssociatedSubEntity(handle, ModelStrategy::R_TimeSignatures), q);
        tracks = new TrackList(strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Children), q);
        workspace = new Workspace(strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Workspace), q);
    }

    void ModelPrivate::handleNotifications() {
        Q_Q(Model);
        QObject::connect(strategy, &ModelStrategy::destroyEntityNotified, q, [=, this](Handle handle) {
            handleEntityDestroyed(handle);
        });
        QObject::connect(strategy, &ModelStrategy::insertIntoSequenceContainerNotified, q, [=, this](Handle sequenceContainerEntity, Handle entity) {
            if (auto sequenceContainerObject = mapToObject(sequenceContainerEntity)) {
                sequenceContainerObject->handleInsertIntoSequenceContainer(entity);
            }
        });

        QObject::connect(strategy, &ModelStrategy::insertIntoListContainerNotified, q, [=, this](Handle listContainerEntity, Handle entity, int index) {
            if (auto listContainerObject = mapToObject(listContainerEntity)) {
                listContainerObject->handleInsertIntoListContainer(entity, index);
            }
        });

        QObject::connect(strategy, &ModelStrategy::insertIntoMapContainerNotified, q, [=, this](Handle mapContainerEntity, Handle entity, const QString &key) {
            if (auto mapContainerObject = mapToObject(mapContainerEntity)) {
                mapContainerObject->handleInsertIntoMapContainer(entity, key);
            }
        });

        QObject::connect(strategy, &ModelStrategy::takeFromContainerNotified, q, [=, this](Handle takenEntity, Handle sequenceContainerEntity, Handle entity) {
            if (auto sequenceContainerObject = mapToObject(sequenceContainerEntity)) {
                sequenceContainerObject->handleTakeFromSequenceContainer(takenEntity, entity);
            }
        });

        QObject::connect(strategy, &ModelStrategy::takeFromListContainerNotified, q, [=, this](Handle takenEntity, Handle listContainerEntity, int index) {
            if (auto listContainerObject = mapToObject(listContainerEntity)) {
                listContainerObject->handleTakeFromListContainer(takenEntity, index);
            }
        });

        QObject::connect(strategy, &ModelStrategy::takeFromMapContainerNotified, q, [=, this](Handle takenEntity, Handle mapContainerEntity, const QString &key) {
            if (auto mapContainerObject = mapToObject(mapContainerEntity)) {
                mapContainerObject->handleTakeFromMapContainer(takenEntity, key);
            }
        });

        QObject::connect(strategy, &ModelStrategy::rotateListContainerNotified, q, [=, this](Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex) {
            if (auto listContainerObject = mapToObject(listContainerEntity)) {
                listContainerObject->handleRotateListContainer(leftIndex, middleIndex, rightIndex);
            }
        });

        QObject::connect(strategy, &ModelStrategy::setEntityPropertyNotified, q, [=, this](Handle entity, ModelStrategy::Property property, const QVariant &value) {
            if (auto entityObject = mapToObject(entity)) {
                entityObject->handleSetEntityProperty(property, value);
            }
        });

        QObject::connect(strategy, &ModelStrategy::spliceDataArrayNotified, q, [=, this](Handle entity, int index, int length, const QVariantList &values) {
            if (auto entityObject = mapToObject(entity)) {
                entityObject->handleSpliceDataArray(index, length, values);
            }
        });

        QObject::connect(strategy, &ModelStrategy::rotateDataArrayNotified, q, [=, this](Handle entity, int leftIndex, int middleIndex, int rightIndex) {
            if (auto entityObject = mapToObject(entity)) {
                entityObject->handleRotateDataArray(leftIndex, middleIndex, rightIndex);
            }
        });
    }

    EntityObject *ModelPrivate::mapToObject(Handle handle) const {
        return objectMap.value(handle);
    }

    Handle ModelPrivate::mapToHandle(EntityObject *object) const {
        return handleMap.value(object);
    }

    template <>
    Clip *ModelPrivate::createObject<Clip>(Handle handle) {
        switch (strategy->getEntityType(handle)) {
            case ModelStrategy::EI_AudioClip:
                return createObject<AudioClip>(handle);
            case ModelStrategy::EI_SingingClip:
                return createObject<SingingClip>(handle);
            default:
                Q_UNREACHABLE();
        }
    }

    template <>
    ParamCurve *ModelPrivate::createObject<ParamCurve>(Handle handle) {
        switch (strategy->getEntityType(handle)) {
            case ModelStrategy::EI_ParamCurveAnchor:
                return createObject<ParamCurveAnchor>(handle);
            case ModelStrategy::EI_ParamCurveFree:
                return createObject<ParamCurveFree>(handle);
            default:
                Q_UNREACHABLE();
        }
    }

    Model::Model(ModelStrategy *strategy, QObject *parent) : EntityObject(parent), d_ptr(new ModelPrivate) {
        Q_D(Model);
        d->q_ptr = this;
        d->strategy = strategy;
        d->init();
        d->handleNotifications();
    }

    Model::~Model() {
        Q_D(Model);
        d->strategy->destroyEntity(handle());
        EntityObject::d_func()->model = nullptr;
    }

    ModelStrategy *Model::strategy() const {
        Q_D(const Model);
        return d->strategy;
    }

    Global *Model::global() const {
        Q_D(const Model);
        return d->global;
    }

    Master *Model::master() const {
        Q_D(const Model);
        return d->master;
    }

    Timeline *Model::timeline() const {
        Q_D(const Model);
        return d->timeline;
    }

    TrackList *Model::tracks() const {
        Q_D(const Model);
        return d->tracks;
    }

    Workspace *Model::workspace() const {
        Q_D(const Model);
        return d->workspace;
    }

    QDspx::Model Model::toQDspx() const {
        QDspx::Model model = {
            .version = QDspx::Model::V1,
            .content = {
                .global = global()->toQDspx(),
                .master = master()->toQDspx(),
                .timeline = timeline()->toQDspx(),
                .tracks = tracks()->toQDspx(),
                .workspace = workspace()->toQDspx(),
            }
        };
        model.content.workspace["diffscope"] = QJsonObject{
            {"loop", QJsonObject{
                {"enabled", timeline()->isLoopEnabled()},
                {"start", timeline()->loopStart()},
                {"length", timeline()->loopLength()},
            }}
        };
        return model;
    }

    void Model::fromQDspx(const QDspx::Model &model) {
        Q_D(Model);
        d->global->fromQDspx(model.content.global);
        d->master->fromQDspx(model.content.master);
        d->timeline->fromQDspx(model.content.timeline);
        d->tracks->fromQDspx(model.content.tracks);
        d->workspace->fromQDspx(model.content.workspace);
        {
            auto loop = model.content.workspace.value("diffscope").value("loop").toObject();
            auto enabled = loop.value("enabled").toBool();
            auto start = loop.value("start").toInt();
            auto length = loop.value("length").toInt();
            if (start < 0 || length <= 0) {
                d->timeline->setLoopEnabled(false);
                d->timeline->setLoopStart(0);
                d->timeline->setLoopLength(1920);
            } else {
                d->timeline->setLoopEnabled(enabled);
                d->timeline->setLoopStart(start);
                d->timeline->setLoopLength(length);
            }
        }
    }

    Label *Model::createLabel() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_Label);
        return d->createObject<Label>(handle);
    }

    Note *Model::createNote() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_Note);
        return d->createObject<Note>(handle);
    }

    Phoneme *Model::createPhoneme() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_Phoneme);
        return d->createObject<Phoneme>(handle);
    }

    Tempo *Model::createTempo() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_Tempo);
        return d->createObject<Tempo>(handle);
    }

    TimeSignature *Model::createTimeSignature() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_TimeSignature);
        return d->createObject<TimeSignature>(handle);
    }

    Track *Model::createTrack() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_Track);
        return d->createObject<Track>(handle);
    }

    WorkspaceInfo *Model::createWorkspaceInfo() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_WorkspaceInfo);
        return d->createObject<WorkspaceInfo>(handle);
    }

    AudioClip *Model::createAudioClip() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_AudioClip);
        return d->createObject<AudioClip>(handle);
    }

    SingingClip *Model::createSingingClip() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_SingingClip);
        return d->createObject<SingingClip>(handle);
    }

    AnchorNode *Model::createAnchorNode() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_ParamCurveAnchorNode);
        return d->createObject<AnchorNode>(handle);
    }

    ParamCurveAnchor *Model::createParamCurveAnchor() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_ParamCurveAnchor);
        return d->createObject<ParamCurveAnchor>(handle);
    }

    ParamCurveFree *Model::createParamCurveFree() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_ParamCurveFree);
        return d->createObject<ParamCurveFree>(handle);
    }

    Param *Model::createParam() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_Param);
        return d->createObject<Param>(handle);
    }

    Source *Model::createSource() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_Source);
        return d->createObject<Source>(handle);
    }

    void Model::destroyItem(EntityObject *object) {
        Q_D(Model);
        auto handle = object->handle();
        object->d_func()->handle = {};
        d->strategy->destroyEntity(handle);
        object->deleteLater();
    }

    void Model::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Model);
        switch (property) {
            case ModelStrategy::P_Name: {
                d->name = value.toString();
                Q_EMIT d->global->nameChanged(d->name);
                break;
            }
            case ModelStrategy::P_Author: {
                d->author = value.toString();
                Q_EMIT d->global->authorChanged(d->author);
                break;
            }
            case ModelStrategy::P_CentShift: {
                d->centShift = value.toInt();
                Q_EMIT d->global->centShiftChanged(d->centShift);
                break;
            }
            case ModelStrategy::P_EditorId: {
                d->editorId = value.toString();
                Q_EMIT d->global->editorIdChanged(d->editorId);
                break;
            }
            case ModelStrategy::P_EditorName: {
                d->editorName = value.toString();
                Q_EMIT d->global->editorNameChanged(d->editorName);
                break;
            }
            case ModelStrategy::P_ControlGain:
            case ModelStrategy::P_ControlPan:
            case ModelStrategy::P_ControlMute: {
                ModelPrivate::proxySetEntityPropertyNotify(d->master, property, value);
                break;
            }
            case ModelStrategy::P_LoopEnabled:
            case ModelStrategy::P_LoopLength:
            case ModelStrategy::P_LoopStart: {
                ModelPrivate::proxySetEntityPropertyNotify(d->timeline, property, value);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_Model.cpp"
