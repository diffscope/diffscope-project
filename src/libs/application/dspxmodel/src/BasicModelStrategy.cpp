#include "BasicModelStrategy.h"

#include <algorithm>

#include <QVariant>
#include <QHash>

namespace dspx {

    class BasicModelStrategyEntity : public QObject {
        Q_OBJECT
    public:
        using QObject::QObject;

        ModelStrategy::Entity type{};

    };

    class BasicModelStrategyItemEntity : public BasicModelStrategyEntity {
        Q_OBJECT
    public:
        using BasicModelStrategyEntity::BasicModelStrategyEntity;

        QHash<ModelStrategy::Property, QVariant> properties;
        QHash<ModelStrategy::Relationship, BasicModelStrategyEntity *> associatedSubEntities;
    };

    class BasicModelStrategySequenceContainerEntity : public BasicModelStrategyEntity {
        Q_OBJECT
    public:
        using BasicModelStrategyEntity::BasicModelStrategyEntity;

        QSet<BasicModelStrategyEntity *> sequence;
    };

    class BasicModelStrategyListContainerEntity : public BasicModelStrategyEntity {
        Q_OBJECT
    public:
        using BasicModelStrategyEntity::BasicModelStrategyEntity;

        QList<BasicModelStrategyEntity *> list;
    };

    class BasicModelStrategyMapContainerEntity : public BasicModelStrategyEntity {
        Q_OBJECT
    public:
        using BasicModelStrategyEntity::BasicModelStrategyEntity;

        QHash<QString, BasicModelStrategyEntity *> map;
    };

    class BasicModelStrategyDataContainerEntity : public BasicModelStrategyEntity {
        Q_OBJECT
    public:
        using BasicModelStrategyEntity::BasicModelStrategyEntity;

        QVariantList data;
    };

    static BasicModelStrategyEntity *createByType(ModelStrategy::Entity type, QObject *parent) {
        BasicModelStrategyEntity *obj;
        switch (type) {
            case ModelStrategy::EI_AudioClip:
            case ModelStrategy::EI_Global:
            case ModelStrategy::EI_Label:
            case ModelStrategy::EI_Note:
            case ModelStrategy::EI_Param:
            case ModelStrategy::EI_ParamCurveAnchor:
            case ModelStrategy::EI_ParamCurveFree:
            case ModelStrategy::EI_ParamCurveAnchorNode:
            case ModelStrategy::EI_Phoneme:
            case ModelStrategy::EI_SingingClip:
            case ModelStrategy::EI_Source:
            case ModelStrategy::EI_Tempo:
            case ModelStrategy::EI_TimeSignature:
            case ModelStrategy::EI_Track:
            case ModelStrategy::EI_WorkspaceInfo:
                obj = new BasicModelStrategyItemEntity(parent);
                obj->type = type;
                break;
            case ModelStrategy::ES_Clips:
            case ModelStrategy::ES_Labels:
            case ModelStrategy::ES_Notes:
            case ModelStrategy::ES_ParamCurveAnchorNodes:
            case ModelStrategy::ES_ParamCurves:
            case ModelStrategy::ES_Tempos:
            case ModelStrategy::ES_TimeSignatures:
                obj = new BasicModelStrategySequenceContainerEntity(parent);
                obj->type = type;
                break;
            case ModelStrategy::EL_Phonemes:
            case ModelStrategy::EL_Tracks:
                obj = new BasicModelStrategyListContainerEntity(parent);
                obj->type = type;
                break;
            case ModelStrategy::ED_ParamCurveFreeItems:
            case ModelStrategy::ED_VibratoPoints:
                obj = new BasicModelStrategyDataContainerEntity(parent);
                obj->type = type;
                break;
            case ModelStrategy::EM_Params:
            case ModelStrategy::EM_Sources:
            case ModelStrategy::EM_Workspace:
                obj = new BasicModelStrategyMapContainerEntity(parent);
                obj->type = type;
                break;
            default:
                Q_UNREACHABLE();
        }
        return obj;
    }

    template <class T>
    T *handleCast(Handle entity) {
        auto obj = qobject_cast<T *>(reinterpret_cast<BasicModelStrategyEntity *>(entity.d));
        Q_ASSERT(obj);
        return obj;
    }

    BasicModelStrategy::BasicModelStrategy(QObject *parent) : ModelStrategy(parent) {
    }

    BasicModelStrategy::~BasicModelStrategy() = default;

    Handle BasicModelStrategy::createEntity(Entity entityType) {
        auto object = createByType(entityType, this);
        Handle entity {reinterpret_cast<quintptr>(object)};
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
        Handle entity {reinterpret_cast<quintptr>(object)};
        Q_EMIT takeFromListContainerNotified(entity, listContainerEntity, index);
        return entity;
    }

    Handle BasicModelStrategy::takeFromMapContainer(Handle mapContainerEntity, const QString &key) {
        auto mapContainerObject = handleCast<BasicModelStrategyMapContainerEntity>(mapContainerEntity);
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
        object->properties.insert(property, value);
        Q_EMIT setEntityPropertyNotified(entity, property, value);
    }

    QVariant BasicModelStrategy::getEntityProperty(Handle entity, Property property) {
        auto object = handleCast<BasicModelStrategyItemEntity>(entity);
        return object->properties.value(property);
    }
    QVariantList BasicModelStrategy::spliceDataContainer(Handle dataContainerEntity, int index, int length, const QVariantList &values) {
        auto &data = handleCast<BasicModelStrategyDataContainerEntity>(dataContainerEntity)->data;
        if (index < 0 || index > data.size() || length < 0 || index + length > data.size()) {
            return {};
        }
        QVariantList removed;
        removed.resize(length);
        std::move(data.begin() + index, data.begin() + index + length, removed.begin());
        auto replaceCount = qMin(length, values.size());
        std::copy(values.begin(), values.begin() + replaceCount, data.begin() + index);
        if (values.size() > length) {
            auto extraSize = values.size() - length;
            auto oldSize = data.size();
            data.resize(oldSize + extraSize);
            std::move_backward(data.begin() + index + length, data.begin() + oldSize, data.begin() + oldSize + extraSize
            );
            std::copy(values.begin() + replaceCount, values.end(), data.begin() + index + replaceCount);
        } else if (values.size() < length) {
            data.remove(index + replaceCount, length - replaceCount);
        }
        Q_EMIT spliceDataContainerNotified(removed, dataContainerEntity, index, length, values);
        return removed;
    }
    QVariantList BasicModelStrategy::sliceDataContainer(Handle dataContainerEntity, int index, int length) {
        const auto &data = handleCast<BasicModelStrategyDataContainerEntity>(dataContainerEntity)->data;
        if (index < 0 || index >= data.size() || length <= 0 || index + length > data.size()) {
            return {};
        }
        return data.mid(index, length);
    }
    int BasicModelStrategy::getSizeOfDataContainer(Handle dataContainerEntity) {
        const auto &data = handleCast<BasicModelStrategyDataContainerEntity>(dataContainerEntity)->data;
        return static_cast<int>(data.size());
    }
    bool BasicModelStrategy::rotateDataContainer(Handle dataContainerEntity, int leftIndex, int middleIndex, int rightIndex) {
        auto &data = handleCast<BasicModelStrategyDataContainerEntity>(dataContainerEntity)->data;
        if (leftIndex < 0 || leftIndex > data.size() || middleIndex < leftIndex || middleIndex > data.size() || rightIndex < middleIndex || rightIndex > data.size()) {
            return false;
        }
        std::rotate(data.begin() + leftIndex, data.begin() + middleIndex, data.begin() + rightIndex);
        Q_EMIT rotateDataContainerNotified(dataContainerEntity, leftIndex, middleIndex, rightIndex);
        return true;
    }

    Handle BasicModelStrategy::getAssociatedSubEntity(Handle entity, Relationship relationship) {
        auto object = handleCast<BasicModelStrategyItemEntity>(entity);
        auto subObject = object->associatedSubEntities.value(relationship);
        if (!subObject) {
            Entity subObjectType;
            switch (relationship) {
                case R_Children:
                    switch (object->type) {
                        case EI_Global:
                            subObjectType = EL_Tracks;
                            break;
                        case EI_Track:
                            subObjectType = ES_Clips;
                            break;
                        case EI_SingingClip:
                            subObjectType = ES_Notes;
                            break;
                        case EI_ParamCurveAnchor:
                            subObjectType = ES_ParamCurveAnchorNodes;
                            break;
                        case EI_ParamCurveFree:
                            subObjectType = ED_ParamCurveFreeItems;
                            break;
                        default:
                            Q_UNREACHABLE();
                    }
                    break;
                case R_Labels:
                    subObjectType = ES_Labels;
                    break;
                case R_ParamCurvesEdited:
                case R_ParamCurvesOriginal:
                case R_ParamCurvesTransform:
                    subObjectType = ES_ParamCurves;
                    break;
                case R_Params:
                    subObjectType = EM_Params;
                    break;
                case R_PhonemesEdited:
                case R_PhonemesOriginal:
                    subObjectType = EL_Phonemes;
                    break;
                case R_Sources:
                    subObjectType = EM_Sources;
                    break;
                case R_Tempos:
                    subObjectType = ES_Tempos;
                    break;
                case R_TimeSignatures:
                    subObjectType = ES_TimeSignatures;
                    break;
                case R_VibratoPointsAmplitude:
                case R_VibratoPointsFrequency:
                    subObjectType = ED_VibratoPoints;
                    break;
                case R_Workspace:
                    subObjectType = EM_Workspace;
                    break;
                default:
                    Q_UNREACHABLE();
            }
            subObject = createByType(subObjectType, object);
            object->associatedSubEntities.insert(relationship, subObject);
        }
        Handle subEntity {reinterpret_cast<quintptr>(subObject)};
        return subEntity;
    }

}

#include "BasicModelStrategy.moc"