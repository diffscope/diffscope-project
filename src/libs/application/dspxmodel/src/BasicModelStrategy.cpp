#include "BasicModelStrategy.h"

#include <algorithm>

#include <QVariant>
#include <QHash>

namespace dspx {

    class BasicModelStrategyEntity : public QObject {
    public:
        using QObject::QObject;

        ModelStrategy::Entity type{};
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
        object->type = entityType;
        Handle entity {reinterpret_cast<quintptr>(object)};
        Q_EMIT createEntityNotified(entity, entityType);
        return entity;
    }

    void BasicModelStrategy::destroyEntity(Handle entity) {
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        delete object;
        Q_EMIT destroyEntityNotified(entity);
    }

    ModelStrategy::Entity BasicModelStrategy::getEntityType(Handle entity) {
        return reinterpret_cast<BasicModelStrategyEntity *>(entity.d)->type;
    }

    bool BasicModelStrategy::insertIntoSequenceContainer(Handle sequenceContainerEntity, Handle entity) {
        auto sequenceContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(sequenceContainerEntity.d);
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
        auto listContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(listContainerEntity.d);
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
        auto mapContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(mapContainerEntity.d);
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

    Handle BasicModelStrategy::takeFromListContainer(Handle listContainerEntity, int index) {
        auto listContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(listContainerEntity.d);
        if (index < 0 || index >= listContainerObject->list.size()) {
            return {};
        }
        auto object = listContainerObject->list.takeAt(index);
        object->setParent(this);
        Handle entity {reinterpret_cast<quintptr>(object)};
        Q_EMIT takeFromListContainerNotified(entity, listContainerEntity, index);
        return entity;
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

    bool BasicModelStrategy::rotateListContainer(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex) {
        auto listContainerObject = reinterpret_cast<BasicModelStrategyEntity *>(listContainerEntity.d);
        if (leftIndex < 0 || leftIndex > listContainerObject->list.size() || middleIndex < leftIndex || middleIndex > listContainerObject->list.size() || rightIndex < middleIndex || rightIndex > listContainerObject->list.size()) {
            return false;
        }
        std::ranges::rotate(listContainerObject->list.begin() + leftIndex, listContainerObject->list.begin() + middleIndex, listContainerObject->list.begin() + rightIndex);
        Q_EMIT rotateListContainerNotified(listContainerEntity, leftIndex, middleIndex, rightIndex);
        return true;
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
        auto subObject = object->associatedSubEntities.value(relationship);
        if (!subObject) {
            subObject = new BasicModelStrategyEntity(object);
            switch (relationship) {
                case R_Children:
                    switch (object->type) {
                        case EI_Global:
                            subObject->type = EL_Tracks;
                            break;
                        case EI_Track:
                            subObject->type = ES_Clips;
                            break;
                        case EI_SingingClip:
                            subObject->type = ES_Notes;
                            break;
                        case EI_ParamCurveAnchor:
                            subObject->type = ES_ParamCurveAnchorNodes;
                            break;
                        case EI_ParamCurveFree:
                            subObject->type = ED_ParamCurveFreeItems;
                            break;
                        default:
                            Q_UNREACHABLE();
                    }
                    break;
                case R_Labels:
                    subObject->type = ES_Labels;
                    break;
                case R_ParamCurvesEdited:
                case R_ParamCurvesOriginal:
                case R_ParamCurvesTransform:
                    subObject->type = ES_ParamCurves;
                    break;
                case R_Params:
                    subObject->type = EM_Params;
                    break;
                case R_PhonemesEdited:
                case R_PhonemesOriginal:
                    subObject->type = EL_Phonemes;
                    break;
                case R_Sources:
                    subObject->type = EM_Sources;
                    break;
                case R_Tempos:
                    subObject->type = ES_Tempos;
                    break;
                case R_TimeSignatures:
                    subObject->type = ES_TimeSignatures;
                    break;
                case R_VibratoPointsAmplitude:
                case R_VibratoPointsFrequency:
                    subObject->type = ED_VibratoPoints;
                    break;
                case R_Workspace:
                    subObject->type = EM_Workspace;
                    break;
                default:
                    Q_UNREACHABLE();
            }
            object->associatedSubEntities.insert(relationship, subObject);
        }
        Handle subEntity {reinterpret_cast<quintptr>(subObject)};
        return subEntity;
    }

}