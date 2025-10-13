#include "Model.h"
#include "Model_p.h"

#include <QVariant>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/Global.h>
#include <dspxmodel/Master.h>
#include <dspxmodel/private/EntityObject_p.h>

namespace dspx {

    void ModelPrivate::handleEntityDestroyed(Handle handle) {
        auto object = objectMap.value(handle);
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