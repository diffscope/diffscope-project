#include "BasicModelStrategy.h"

#include <algorithm>

#include <QVariant>
#include <QHash>

namespace dspx {

    class BasicModelStrategyEntity : public QObject {
    public:
        explicit BasicModelStrategyEntity(QObject *parent = nullptr);
        ~BasicModelStrategyEntity() override;

        QHash<ModelStrategy::Property, QVariant> properties;
        QList<BasicModelStrategyEntity *> list;
        QSet<BasicModelStrategyEntity *> sequence;
        QHash<QString, BasicModelStrategyEntity *> map;
        QHash<ModelStrategy::Relationship, BasicModelStrategyEntity *> associatedSubEntities;
    };

    BasicModelStrategy::BasicModelStrategy(QObject *parent) : ModelStrategy(parent) {
    }

    BasicModelStrategy::~BasicModelStrategy() = default;

    Handle BasicModelStrategy::createEntity(Entity entityType) {
        auto object = new BasicModelStrategyEntity(this);
        Handle entity {reinterpret_cast<quintptr>(object)};
        Q_EMIT createEntityNotified(entity, entityType);
        return entity;
    }

    void BasicModelStrategy::destroyEntity(Handle entity) {
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        delete object;
        Q_EMIT destroyEntityNotified(entity);
    }

    void BasicModelStrategy::insertIntoSequenceContainer(Handle sequenceContainerEntity, Handle entity) {
        auto sequenceContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(sequenceContainerEntity.d);
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        if (!sequenceContainerObject->sequence.contains(object)) {
            sequenceContainerObject->sequence.insert(object);
            object->setParent(sequenceContainerObject);
            Q_EMIT insertIntoSequenceContainerNotified(sequenceContainerEntity, entity);
        }
    }

    void BasicModelStrategy::insertIntoListContainer(Handle listContainerEntity, const QList<Handle> &entities, int index) {
        auto listContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(listContainerEntity.d);
        if (index < 0 || index > listContainerObject->list.size()) {
            return;
        }
        QList<Handle> insertedEntities;
        for (auto entity : entities) {
            auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
            if (object->parent() != listContainerObject) {
                listContainerObject->list.insert(index, object);
                object->setParent(listContainerObject);
                insertedEntities.append(entity);
                index++;
            }
        }
        Q_EMIT insertIntoListContainerNotified(listContainerEntity, insertedEntities, index);
    }

    void BasicModelStrategy::insertIntoMapContainer(Handle mapContainerEntity, Handle entity, const QString &key) {
        auto mapContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(mapContainerEntity.d);
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        if (object->parent() == mapContainerObject) {
            return;
        }
        if (mapContainerObject->map.value(key) != object) {
            mapContainerObject->map.insert(key, object);
            object->setParent(mapContainerObject);
            Q_EMIT insertIntoMapContainerNotified(mapContainerEntity, entity, key);
        }
    }

    Handle BasicModelStrategy::takeFromSequenceContainer(Handle sequenceContainerEntity, Handle entity) {
        auto sequenceContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(sequenceContainerEntity.d);
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        if (sequenceContainerObject->sequence.contains(object)) {
            sequenceContainerObject->sequence.remove(object);
            object->setParent(this);
            Q_EMIT takeFromContainerNotified(entity, sequenceContainerEntity, entity);
            return entity;
        }
        return {};
    }

    QList<Handle> BasicModelStrategy::takeFromListContainer(Handle listContainerEntity, const QList<int> &indexes) {
        auto listContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(listContainerEntity.d);
        QList<Handle> takenEntities;
        QList<int> takenIndexes;
        QList<int> sortedIndexes = indexes;
        std::ranges::sort(sortedIndexes, std::greater());
        for (auto index : sortedIndexes) {
            if (index < 0 || index >= listContainerObject->list.size()) {
                continue;
            }
            auto object = listContainerObject->list.takeAt(index);
            object->setParent(this);
            Handle entity {reinterpret_cast<quintptr>(object)};
            takenEntities.append(entity);
            takenIndexes.append(index);
        }
        Q_EMIT takeFromListContainerNotified(takenEntities, listContainerEntity, takenIndexes);
        return takenEntities;
    }

    Handle BasicModelStrategy::takeFromMapContainer(Handle mapContainerEntity, const QString &key) {
        auto mapContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(mapContainerEntity.d);
        if (mapContainerObject->map.contains(key)) {
            auto object = mapContainerObject->map.take(key);
            object->setParent(this);
            Handle entity {reinterpret_cast<quintptr>(object)};
            Q_EMIT takeFromMapContainerNotified(entity, mapContainerEntity, key);
            return entity;
        }
        return {};
    }

    void BasicModelStrategy::rotateListContainer(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex) {
        auto listContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(listContainerEntity.d);
        if (leftIndex < 0 || leftIndex > listContainerObject->list.size() || middleIndex < leftIndex || middleIndex > listContainerObject->list.size() || rightIndex < middleIndex || rightIndex > listContainerObject->list.size()) {
            return;
        }
        std::ranges::rotate(listContainerObject->list.begin() + leftIndex, listContainerObject->list.begin() + middleIndex, listContainerObject->list.begin() + rightIndex);
        Q_EMIT rotateListContainerNotified(listContainerEntity, leftIndex, middleIndex, rightIndex);
    }

    void BasicModelStrategy::setEntityProperty(Handle entity, Property property, const QVariant &value) {
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        object->properties.insert(property, value);
        Q_EMIT setEntityPropertyNotified(entity, property, value);
    }

    QVariant BasicModelStrategy::getEntityProperty(Handle entity, Property property) {
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        return object->properties.value(property);
    }

    Handle BasicModelStrategy::getAssociatedSubEntity(Handle entity, Relationship relationship) {
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        auto subObject = object->associatedSubEntities.contains(relationship) ?
            object->associatedSubEntities.value(relationship) :
            new BasicModelStrategyEntity(object);
        object->associatedSubEntities.insert(relationship, subObject);
        Handle subEntity {reinterpret_cast<quintptr>(subObject)};
        return subEntity;
    }

}