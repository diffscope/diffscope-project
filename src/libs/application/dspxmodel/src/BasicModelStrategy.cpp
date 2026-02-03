#include "BasicModelStrategy.h"

#include <algorithm>

#include <dspxmodel/private/SpliceHelper_p.h>
#include <dspxmodel/private/BasicModelStrategyEntity_p.h>

namespace dspx {

    BasicModelStrategy::BasicModelStrategy(QObject *parent) : ModelStrategy(parent) {
    }

    BasicModelStrategy::~BasicModelStrategy() = default;

    Handle BasicModelStrategy::createEntity(Entity entityType) {
        auto object = BasicModelStrategyEntity::createByType(entityType, this);
        Handle entity{reinterpret_cast<quintptr>(object)};
        Q_EMIT createEntityNotified(entity, entityType);
        return entity;
    }

    void BasicModelStrategy::destroyEntity(Handle entity) {
        auto object = handleCast<BasicModelStrategyEntity>(entity);
        delete object;
        Q_EMIT destroyEntityNotified(entity);
    }

    ModelStrategy::Entity BasicModelStrategy::getEntityType(Handle entity) {
        return reinterpret_cast<BasicModelStrategyEntity *>(entity.d)->type;
    }

    QList<Handle> BasicModelStrategy::getEntitiesFromSequenceContainer(Handle sequenceContainerEntity) {
        QList<Handle> a;
        std::ranges::transform(handleCast<BasicModelStrategySequenceContainerEntity>(sequenceContainerEntity)->sequence, std::back_inserter(a), [](auto *obj) {
            return Handle{reinterpret_cast<quintptr>(obj)};
        });
        return a;
    }

    QList<Handle> BasicModelStrategy::getEntitiesFromListContainer(Handle listContainerEntity) {
        QList<Handle> a;
        std::ranges::transform(handleCast<BasicModelStrategyListContainerEntity>(listContainerEntity)->list, std::back_inserter(a), [](auto *obj) {
            return Handle{reinterpret_cast<quintptr>(obj)};
        });
        return a;
    }

    QList<QPair<QString, Handle>> BasicModelStrategy::getEntitiesFromMapContainer(Handle mapContainerEntity) {
        QList<QPair<QString, Handle>> a;
        for (auto [key, value] : handleCast<BasicModelStrategyMapContainerEntity>(mapContainerEntity)->map.asKeyValueRange()) {
            a.append({key, Handle{reinterpret_cast<quintptr>(value)}});
        }
        return a;
    }

    bool BasicModelStrategy::insertIntoSequenceContainer(Handle sequenceContainerEntity, Handle entity) {
        auto sequenceContainerObject = handleCast<BasicModelStrategySequenceContainerEntity>(sequenceContainerEntity);
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        if (object->parent() != this) {
            return false;
        }
        sequenceContainerObject->sequence.insert(object);
        object->setParent(sequenceContainerObject);
        Q_EMIT insertIntoSequenceContainerNotified(sequenceContainerEntity, entity);
        return true;
    }

    bool BasicModelStrategy::insertIntoListContainer(Handle listContainerEntity, Handle entity, int index) {
        auto listContainerObject = handleCast<BasicModelStrategyListContainerEntity>(listContainerEntity);
        if (index < 0 || index > listContainerObject->list.size()) {
            return false;
        }
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        if (object->parent() != this) {
            return false;
        }
        listContainerObject->list.insert(index, object);
        object->setParent(listContainerObject);
        Q_EMIT insertIntoListContainerNotified(listContainerEntity, entity, index);
        return true;
    }

    bool BasicModelStrategy::insertIntoMapContainer(Handle mapContainerEntity, Handle entity, const QString &key) {
        auto mapContainerObject = handleCast<BasicModelStrategyMapContainerEntity>(mapContainerEntity);
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        if (object->parent() != this) {
            return false;
        }
        if (mapContainerObject->map.contains(key)) {
            auto handle = takeFromMapContainer(mapContainerEntity, key);
            destroyEntity(handle);
        }
        mapContainerObject->map.insert(key, object);
        object->setParent(mapContainerObject);
        Q_EMIT insertIntoMapContainerNotified(mapContainerEntity, entity, key);
        return true;
    }

    bool BasicModelStrategy::moveToAnotherSequenceContainer(Handle sequenceContainerEntity, Handle entity, Handle otherSequenceContainerEntity) {
        auto sequenceContainerObject = handleCast<BasicModelStrategySequenceContainerEntity>(sequenceContainerEntity);
        auto otherSequenceContainerObject = handleCast<BasicModelStrategySequenceContainerEntity>(otherSequenceContainerEntity);
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        if (!sequenceContainerObject->sequence.contains(object)) {
            return false;
        }
        sequenceContainerObject->sequence.remove(object);
        otherSequenceContainerObject->sequence.insert(object);
        object->setParent(otherSequenceContainerObject);
        Q_EMIT moveToAnotherSequenceContainerNotified(sequenceContainerEntity, entity, otherSequenceContainerEntity);
        return true;
    }

    Handle BasicModelStrategy::takeFromSequenceContainer(Handle sequenceContainerEntity, Handle entity) {
        auto sequenceContainerObject = handleCast<BasicModelStrategySequenceContainerEntity>(sequenceContainerEntity);
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        if (sequenceContainerObject->sequence.contains(object)) {
            sequenceContainerObject->sequence.remove(object);
            object->setParent(this);
            Q_EMIT takeFromContainerNotified(entity, sequenceContainerEntity, entity);
            return entity;
        }
        return {};
    }

    Handle BasicModelStrategy::takeFromListContainer(Handle listContainerEntity, int index) {
        auto listContainerObject = handleCast<BasicModelStrategyListContainerEntity>(listContainerEntity);
        if (index < 0 || index >= listContainerObject->list.size()) {
            return {};
        }
        auto object = listContainerObject->list.takeAt(index);
        object->setParent(this);
        Handle entity{reinterpret_cast<quintptr>(object)};
        Q_EMIT takeFromListContainerNotified(entity, listContainerEntity, index);
        return entity;
    }

    Handle BasicModelStrategy::takeFromMapContainer(Handle mapContainerEntity, const QString &key) {
        auto mapContainerObject = handleCast<BasicModelStrategyMapContainerEntity>(mapContainerEntity);
        if (mapContainerObject->map.contains(key)) {
            auto object = mapContainerObject->map.take(key);
            object->setParent(this);
            Handle entity{reinterpret_cast<quintptr>(object)};
            Q_EMIT takeFromMapContainerNotified(entity, mapContainerEntity, key);
            return entity;
        }
        return {};
    }

    bool BasicModelStrategy::rotateListContainer(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex) {
        auto listContainerObject = handleCast<BasicModelStrategyListContainerEntity>(listContainerEntity);
        if (leftIndex < 0 || leftIndex > listContainerObject->list.size() || middleIndex < leftIndex || middleIndex > listContainerObject->list.size() || rightIndex < middleIndex || rightIndex > listContainerObject->list.size()) {
            return false;
        }
        std::rotate(listContainerObject->list.begin() + leftIndex, listContainerObject->list.begin() + middleIndex, listContainerObject->list.begin() + rightIndex);
        Q_EMIT rotateListContainerNotified(listContainerEntity, leftIndex, middleIndex, rightIndex);
        return true;
    }

    void BasicModelStrategy::setEntityProperty(Handle entity, Property property, const QVariant &value) {
        auto object = handleCast<BasicModelStrategyItemEntity>(entity);
        Q_ASSERT(isEntityTypeAndPropertyTypeCompatible(object->type, property));
        object->properties.insert(property, value);
        Q_EMIT setEntityPropertyNotified(entity, property, value);
    }

    QVariant BasicModelStrategy::getEntityProperty(Handle entity, Property property) {
        auto object = handleCast<BasicModelStrategyItemEntity>(entity);
        Q_ASSERT(isEntityTypeAndPropertyTypeCompatible(object->type, property));
        return object->properties.value(property);
    }
    bool BasicModelStrategy::spliceDataArray(Handle dataArrayEntity, int index, int length, const QVariantList &values) {
        auto &data = handleCast<BasicModelStrategyDataArrayEntity>(dataArrayEntity)->data;
        if (index < 0 || index > data.size() || length < 0 || index + length > data.size()) {
            return false;
        }
        SpliceHelper::splice(data, data.begin() + index, data.begin() + index + length, values.begin(), values.end());
        Q_EMIT spliceDataArrayNotified(dataArrayEntity, index, length, values);
        return true;
    }
    QVariantList BasicModelStrategy::sliceDataArray(Handle dataArrayEntity, int index, int length) {
        const auto &data = handleCast<BasicModelStrategyDataArrayEntity>(dataArrayEntity)->data;
        if (index < 0 || index >= data.size()) {
            return {};
        }
        return data.mid(index, length);
    }
    int BasicModelStrategy::getSizeOfDataArray(Handle dataArrayEntity) {
        const auto &data = handleCast<BasicModelStrategyDataArrayEntity>(dataArrayEntity)->data;
        return static_cast<int>(data.size());
    }
    bool BasicModelStrategy::rotateDataArray(Handle dataArrayEntity, int leftIndex, int middleIndex, int rightIndex) {
        auto &data = handleCast<BasicModelStrategyDataArrayEntity>(dataArrayEntity)->data;
        if (leftIndex < 0 || leftIndex > data.size() || middleIndex < leftIndex || middleIndex > data.size() || rightIndex < middleIndex || rightIndex > data.size()) {
            return false;
        }
        std::rotate(data.begin() + leftIndex, data.begin() + middleIndex, data.begin() + rightIndex);
        Q_EMIT rotateDataArrayNotified(dataArrayEntity, leftIndex, middleIndex, rightIndex);
        return true;
    }

    Handle BasicModelStrategy::getAssociatedSubEntity(Handle entity, Relationship relationship) {
        auto object = handleCast<BasicModelStrategyItemEntity>(entity);
        auto subObject = object->associatedSubEntities.value(relationship);
        if (!subObject) {
            Entity subObjectType = getAssociatedSubEntityTypeFromEntityTypeAndRelationship(object->type, relationship);
            subObject = BasicModelStrategyEntity::createByType(subObjectType, object);
            object->associatedSubEntities.insert(relationship, subObject);
        }
        Handle subEntity{reinterpret_cast<quintptr>(subObject)};
        return subEntity;
    }

}
