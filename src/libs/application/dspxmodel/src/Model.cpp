#include "Model.h"
#include "Model_p.h"

#include <QVariant>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/Global.h>
#include <dspxmodel/Master.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/private/EntityObject_p.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/WorkspaceInfo.h>
#include <dspxmodel/Workspace.h>

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
        gain = strategy->getEntityProperty(handle, ModelStrategy::P_ControlGain).toDouble();
        pan = strategy->getEntityProperty(handle, ModelStrategy::P_ControlPan).toDouble();
        mute = strategy->getEntityProperty(handle, ModelStrategy::P_ControlMute).toBool();

        global = new Global(q);
        master = new Master(q);
        timeline = new Timeline(q);

        labels = new LabelSequence(strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Labels), q);
        workspace = new Workspace(strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Workspace), q);
    }

    EntityObject * ModelPrivate::mapToObject(Handle handle) const {
        return objectMap.value(handle);
    }

    Handle ModelPrivate::mapToHandle(EntityObject *object) const {
        return handleMap.value(object);
    }

    Model::Model(ModelStrategy *strategy, QObject *parent) : EntityObject(parent), d_ptr(new ModelPrivate) {
        Q_D(Model);
        d->strategy = strategy;
        d->init();
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

    TrackList *Model::trackList() const {
        Q_D(const Model);
        return d->trackList;
    }

    Workspace *Model::workspace() const {
        Q_D(const Model);
        return d->workspace;
    }

    QDspx::Model Model::toQDspx() const {
        return {
            QStringLiteral("1.0.0"),
            {
                global()->toQDspx(),
                master()->toQDspx(),
                timeline()->toQDspx(),
                // TODO track
                {},
                workspace()->toQDspx(),
            }
        };
    }

    void Model::fromQDspx(const QDspx::Model &model) {
        Q_D(Model);
        d->global->fromQDspx(model.content.global);
        d->master->fromQDspx(model.content.master);
        d->timeline->fromQDspx(model.content.timeline);
        // TODO tracks
        d->workspace->fromQDspx(model.content.workspace);
    }

    Label *Model::createLabel() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_Label);
        return d->createObject<Label>(handle);
    }

    WorkspaceInfo * Model::createWorkspaceInfo() {
        Q_D(Model);
        auto handle = d->strategy->createEntity(ModelStrategy::EI_WorkspaceInfo);
        return d->createObject<WorkspaceInfo>(handle);
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
            case ModelStrategy::P_ControlGain: {
                d->gain = value.toDouble();
                Q_EMIT d->master->gainChanged(d->gain);
                break;
            }
            case ModelStrategy::P_ControlPan: {
                d->pan = value.toDouble();
                Q_EMIT d->master->panChanged(d->pan);
                break;
            }
            case ModelStrategy::P_ControlMute: {
                d->mute = value.toBool();
                Q_EMIT d->master->muteChanged(d->mute);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_Model.cpp"