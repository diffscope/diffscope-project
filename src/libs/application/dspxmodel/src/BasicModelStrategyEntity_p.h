#ifndef DIFFSCOPE_DSPX_MODEL_BASICMODELSTRATEGYENTITY_P_H
#define DIFFSCOPE_DSPX_MODEL_BASICMODELSTRATEGYENTITY_P_H

#include <QHash>
#include <QVariant>
#include <QList>

#include <dspxmodel/ModelStrategy.h>

namespace dspx {
    class BasicModelStrategyEntity : public QObject {
        Q_OBJECT
    public:
        using QObject::QObject;

        ModelStrategy::Entity type{};

        static inline BasicModelStrategyEntity *createByType(ModelStrategy::Entity type, QObject *parent);
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

    class BasicModelStrategyDataArrayEntity : public BasicModelStrategyEntity {
        Q_OBJECT
    public:
        using BasicModelStrategyEntity::BasicModelStrategyEntity;

        QVariantList data;
    };

    BasicModelStrategyEntity *BasicModelStrategyEntity::createByType(ModelStrategy::Entity type, QObject *parent) {
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
            case ModelStrategy::ED_ParamCurveFreeValues:
            case ModelStrategy::ED_VibratoPoints:
                obj = new BasicModelStrategyDataArrayEntity(parent);
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
}

#endif //DIFFSCOPE_DSPX_MODEL_BASICMODELSTRATEGYENTITY_P_H
