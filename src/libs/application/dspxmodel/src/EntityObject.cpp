#include "EntityObject.h"
#include "EntityObject_p.h"

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    EntityObject::EntityObject(Handle handle, Model *model) : EntityObject(model) {
        Q_D(EntityObject);
        d->handle = handle;
        d->model = model;
        auto pModel = ModelPrivate::get(model);
        pModel->objectMap.insert(handle, this);
        pModel->handleMap.insert(this, handle);
    }
    EntityObject::EntityObject(QObject *parent) : QObject(parent), d_ptr(new EntityObjectPrivate) {
        Q_D(EntityObject);
        d->q_ptr = this;
    }
    EntityObject::~EntityObject() {
        Q_D(EntityObject);
        if (d->model && d->handle) {
            d->model->strategy()->destroyEntity(d->handle);
        }
    }

    Model *EntityObject::model() const {
        Q_D(const EntityObject);
        return d->model;
    }

    Handle EntityObject::handle() const {
        Q_D(const EntityObject);
        return d->handle;
    }

    void EntityObject::handleInsertIntoSequenceContainer(Handle entity) {
        Q_UNREACHABLE();
    }
    void EntityObject::handleInsertIntoListContainer(Handle entity, int index) {
        Q_UNREACHABLE();
    }
    void EntityObject::handleInsertIntoMapContainer(Handle entity, const QString &key) {
        Q_UNREACHABLE();
    }
    void EntityObject::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_UNREACHABLE();
    }
    void EntityObject::handleTakeFromListContainer(Handle takenEntity, int index) {
        Q_UNREACHABLE();
    }
    void EntityObject::handleTakeFromMapContainer(Handle takenEntity, const QString &key) {
        Q_UNREACHABLE();
    }
    void EntityObject::handleRotateListContainer(int leftIndex, int middleIndex, int rightIndex) {
        Q_UNREACHABLE();
    }
    void EntityObject::handleSetEntityProperty(int property, const QVariant &value) {
        Q_UNREACHABLE();
    }
    void EntityObject::handleSpliceDataArray(int index, int length, const QVariantList &values) {
        Q_UNREACHABLE();
    }
    void EntityObject::handleRotateDataArray(int leftIndex, int middleIndex, int rightIndex) {
        Q_UNREACHABLE();
    }
}

#include "moc_EntityObject.cpp"
