#ifndef DIFFSCOPE_DSPX_MODEL_MODELSTRATEGY_H
#define DIFFSCOPE_DSPX_MODEL_MODELSTRATEGY_H

#include <vector>

#include <QObject>
#include <QVariant>

#include <dspxmodel/Handle.h>

namespace dspx {

    class DSPX_MODEL_EXPORT ModelStrategy : public QObject {
        Q_OBJECT
    public:
        explicit ModelStrategy(QObject *parent = nullptr);
        ~ModelStrategy() override;

        enum Entity {
            EI_AudioClip,
            EI_Global,
            EI_Label,
            EI_Note,
            EI_Param,
            EI_ParamCurveAnchor,
            EI_ParamCurveFree,
            EI_ParamCurveAnchorNode,
            EI_Phoneme,
            EI_SingingClip,
            EI_Source,
            EI_Tempo,
            EI_TimeSignature,
            EI_Track,
            EI_WorkspaceInfo,

            ES_Clips,
            ES_Labels,
            ES_Notes,
            ES_ParamCurveAnchorNodes,
            ES_ParamCurves,
            ES_Tempos,
            ES_TimeSignatures,

            EL_Phonemes,
            EL_Tracks,

            ED_ParamCurveFreeValues,
            ED_VibratoPoints,

            EM_Params,
            EM_Sources,
            EM_Workspace,
        };

        enum Property {
            P_Author,
            P_CentShift,
            P_ClipStart,
            P_ClipLength,
            P_ColorId,
            P_ControlGain,
            P_ControlMute,
            P_ControlPan,
            P_ControlRecord,
            P_ControlSolo,
            P_Denominator,
            P_EditorId,
            P_EditorName,
            P_Height,
            P_JsonObject,
            P_KeyNumber,
            P_Language,
            P_Length,
            P_LoopEnabled,
            P_LoopLength,
            P_LoopStart,
            P_Measure,
            P_Name,
            P_Numerator,
            P_Onset,
            P_Path,
            P_Position,
            P_PronunciationEdited,
            P_PronunciationOriginal,
            P_Text,
            P_Type,
            P_Value,
            P_VibratoAmplitude,
            P_VibratoEnd,
            P_VibratoFrequency,
            P_VibratoOffset,
            P_VibratoPhase,
            P_VibratoStart,
        };

        enum Relationship {
            R_Children,
            R_Labels,
            R_ParamCurvesEdited,
            R_ParamCurvesOriginal,
            R_ParamCurvesTransform,
            R_Params,
            R_PhonemesEdited,
            R_PhonemesOriginal,
            R_Sources,
            R_Tempos,
            R_TimeSignatures,
            R_VibratoPointsAmplitude,
            R_VibratoPointsFrequency,
            R_Workspace,
        };

        struct PropertySpec {
            Property propertyType;
            QMetaType::Type metaType;
            bool (*validate)(const QVariant &value);

            PropertySpec(Property propertyType, QMetaType::Type metaType, bool (*validate)(const QVariant &value) = [](const QVariant &) { return true; })
                : propertyType(propertyType), metaType(metaType), validate(validate) {
            }
        };

        static inline Entity getAssociatedSubEntityTypeFromEntityTypeAndRelationship(Entity entityType, Relationship relationship);
        static inline std::vector<PropertySpec> getEntityPropertySpecsFromEntityType(Entity entityType);
        static inline PropertySpec getPropertySpecFromEntityTypeAndPropertyType(Entity entityType, Property propertyType);
        static inline bool isEntityTypeAndPropertyTypeCompatible(Entity entityType, Property propertyType);

        virtual Handle createEntity(Entity entityType) = 0;
        virtual void destroyEntity(Handle entity) = 0;
        virtual Entity getEntityType(Handle entity) = 0;

        virtual QList<Handle> getEntitiesFromSequenceContainer(Handle sequenceContainerEntity) = 0;
        virtual QList<Handle> getEntitiesFromListContainer(Handle listContainerEntity) = 0;
        virtual QList<QPair<QString, Handle>> getEntitiesFromMapContainer(Handle mapContainerEntity) = 0;

        virtual bool insertIntoSequenceContainer(Handle sequenceContainerEntity, Handle entity) = 0;
        virtual bool insertIntoListContainer(Handle listContainerEntity, Handle entity, int index) = 0;
        virtual bool insertIntoMapContainer(Handle mapContainerEntity, Handle entity, const QString &key) = 0;

        virtual Handle takeFromSequenceContainer(Handle sequenceContainerEntity, Handle entity) = 0;
        virtual Handle takeFromListContainer(Handle listContainerEntity, int index) = 0;
        virtual Handle takeFromMapContainer(Handle mapContainerEntity, const QString &key) = 0;
        virtual bool rotateListContainer(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex) = 0;

        virtual void setEntityProperty(Handle entity, Property property, const QVariant &value) = 0;
        virtual QVariant getEntityProperty(Handle entity, Property property) = 0;

        virtual bool spliceDataArray(Handle dataContainerEntity, int index, int length, const QVariantList &values) = 0;
        virtual QVariantList sliceDataArray(Handle dataContainerEntity, int index, int length) = 0;
        virtual int getSizeOfDataArray(Handle dataContainerEntity) = 0;
        virtual bool rotateDataArray(Handle dataContainerEntity, int leftIndex, int middleIndex, int rightIndex) = 0;

        virtual Handle getAssociatedSubEntity(Handle entity, Relationship relationship) = 0;

    Q_SIGNALS:
        void createEntityNotified(Handle entity, Entity entityType);
        void destroyEntityNotified(Handle entity);

        void insertIntoSequenceContainerNotified(Handle sequenceContainerEntity, Handle entity);
        void insertIntoListContainerNotified(Handle listContainerEntity, Handle entity, int index);
        void insertIntoMapContainerNotified(Handle mapContainerEntity, Handle entity, const QString &key);

        void takeFromContainerNotified(Handle takenEntity, Handle sequenceContainerEntity, Handle entity);
        void takeFromListContainerNotified(Handle takenEntities, Handle listContainerEntity, int index);
        void takeFromMapContainerNotified(Handle takenEntity, Handle mapContainerEntity, const QString &key);
        void rotateListContainerNotified(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex);

        void setEntityPropertyNotified(Handle entity, Property property, const QVariant &value);

        void spliceDataArrayNotified(Handle dataContainerEntity, int index, int length, const QVariantList &values);
        void rotateDataArrayNotified(Handle dataContainerEntity, int leftIndex, int middleIndex, int rightIndex);
    };

    ModelStrategy::Entity ModelStrategy::getAssociatedSubEntityTypeFromEntityTypeAndRelationship(Entity entityType, Relationship relationship) {
        if (relationship == R_Children) {
            if (entityType == EI_Global) {
                return EL_Tracks;
            }
            if (entityType == EI_Track) {
                return ES_Clips;
            }
            if (entityType == EI_SingingClip) {
                return ES_Notes;
            }
            if (entityType == EI_ParamCurveAnchor) {
                return ES_ParamCurveAnchorNodes;
            }
            if (entityType == EI_ParamCurveFree) {
                return ED_ParamCurveFreeValues;
            }
        } else if (relationship == R_Labels) {
            if (entityType == EI_Global) {
                return ES_Labels;
            }
        } else if (relationship == R_ParamCurvesEdited || relationship == R_ParamCurvesOriginal || relationship == R_ParamCurvesTransform) {
            if (entityType == EI_Param) {
                return ES_ParamCurves;
            }
        } else if (relationship == R_Params) {
            if (entityType == EI_SingingClip) {
                return EM_Params;
            }
        } else if (relationship == R_PhonemesEdited || relationship == R_PhonemesOriginal) {
            if (entityType == EI_Note) {
                return EL_Phonemes;
            }
        } else if (relationship == R_Sources) {
            if (entityType == EI_SingingClip) {
                return EM_Sources;
            }
        } else if (relationship == R_Tempos) {
            if (entityType == EI_Global) {
                return ES_Tempos;
            }
        } else if (relationship == R_TimeSignatures) {
            if (entityType == EI_Global) {
                return ES_TimeSignatures;
            }
        } else if (relationship == R_VibratoPointsAmplitude || relationship == R_VibratoPointsFrequency) {
            if (entityType == EI_Note) {
                return ED_VibratoPoints;
            }
        } else if (relationship == R_Workspace) {
            if (entityType == EI_Global || entityType == EI_Track || entityType == EI_AudioClip || entityType == EI_SingingClip || entityType == EI_Note) {
                return EM_Workspace;
            }
        }
        Q_UNREACHABLE();
    }

    std::vector<ModelStrategy::PropertySpec> ModelStrategy::getEntityPropertySpecsFromEntityType(Entity entityType) {
        static auto validateCentShift = [](const QVariant &value) {
            auto v = value.toInt();
            return v >= -50 && v <= 50;
        };
        static auto validatePan = [](const QVariant &value) {
            auto v = value.toDouble();
            return v >= -1 && v <= 1;
        };
        static auto validateIntGreaterOrEqualZero = [](const QVariant &value) {
            auto v = value.toInt();
            return v >= 0;
        };
        static auto validateIntGreaterZero = [](const QVariant &value) {
            auto v = value.toInt();
            return v > 0;
        };
        static auto validateDoubleGreaterOrEqualZero = [](const QVariant &value) {
            auto v = value.toDouble();
            return v >= 0;
        };
        static auto validateDoubleBetweenZeroAndOne = [](const QVariant &value) {
            auto v = value.toDouble();
            return v >= 0 && v <= 1;
        };
        static auto validateKeyNumber = [](const QVariant &value) {
            auto v = value.toInt();
            return v >= 0 && v <= 127;
        };
        static auto validateTempo = [](const QVariant &value) {
            auto v = value.toDouble();
            return v >= 10 && v <= 1000;
        };
        static auto validateTimeSignatureDenominator = [](const QVariant &value) {
            auto v = value.toInt();
            return v == 1 || v == 2 || v == 4 || v == 8 || v == 16 || v == 32 || v == 64 || v == 128;
        };
        static auto validateTimeSignatureNumerator = [](const QVariant &value) {
            auto v = value.toInt();
            return v >= 1;
        };
        switch (entityType) {
            case EI_AudioClip: return {
                {P_Name, QMetaType::QString},
                {P_ControlGain, QMetaType::Double},
                {P_ControlPan, QMetaType::Double, validatePan},
                {P_ControlMute, QMetaType::Bool},
                {P_Position, QMetaType::Int},
                {P_Length, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_ClipStart, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_ClipLength, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_Path, QMetaType::QString}
            };
            case EI_Global: return {
                {P_Name, QMetaType::QString},
                {P_Author, QMetaType::QString},
                {P_CentShift, QMetaType::Int, validateCentShift},
                {P_EditorId, QMetaType::QString},
                {P_EditorName, QMetaType::QString},
                {P_ControlGain, QMetaType::Double},
                {P_ControlPan, QMetaType::Double, validatePan},
                {P_ControlMute, QMetaType::Bool},
                {P_LoopEnabled, QMetaType::Bool},
                {P_LoopStart, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_LoopLength, QMetaType::Int, validateIntGreaterZero},
            };
            case EI_Label: return {
                {P_Position, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_Text, QMetaType::QString}
            };
            case EI_Note: return {
                {P_CentShift, QMetaType::Int, validateCentShift},
                {P_KeyNumber, QMetaType::Int, validateKeyNumber},
                {P_Language, QMetaType::QString},
                {P_Length, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_Text, QMetaType::QString},
                {P_Position, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_PronunciationOriginal, QMetaType::QString},
                {P_PronunciationEdited, QMetaType::QString},
                {P_VibratoAmplitude, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_VibratoEnd, QMetaType::Double, validateDoubleBetweenZeroAndOne},
                {P_VibratoFrequency, QMetaType::Double, validateDoubleGreaterOrEqualZero},
                {P_VibratoOffset, QMetaType::Int},
                {P_VibratoPhase, QMetaType::Double, validateDoubleBetweenZeroAndOne},
                {P_VibratoStart, QMetaType::Double, validateDoubleBetweenZeroAndOne}
            };
            case EI_Param: return {};
            case EI_ParamCurveAnchor: return {
                {P_Position, QMetaType::Int}
            };
            case EI_ParamCurveFree: return {
                {P_Position, QMetaType::Int}
            };
            case EI_ParamCurveAnchorNode: return {
                {P_Type, QMetaType::Int},
                {P_Position, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_Value, QMetaType::Int}
            };
            case EI_Phoneme: return {
                {P_Language, QMetaType::QString},
                {P_Position, QMetaType::Int},
                {P_Text, QMetaType::QString},
                {P_Onset, QMetaType::Bool}
            };
            case EI_SingingClip: return {
                {P_Name, QMetaType::QString},
                {P_ControlGain, QMetaType::Double},
                {P_ControlPan, QMetaType::Double, validatePan},
                {P_ControlMute, QMetaType::Bool},
                {P_Position, QMetaType::Int},
                {P_Length, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_ClipStart, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_ClipLength, QMetaType::Int, validateIntGreaterOrEqualZero}
            };
            case EI_Source: return {
                {P_JsonObject, QMetaType::QJsonObject}
            };
            case EI_Tempo: return {
                {P_Position, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_Value, QMetaType::Double, validateTempo}
            };
            case EI_TimeSignature: return {
                {P_Measure, QMetaType::Int, validateIntGreaterOrEqualZero},
                {P_Numerator, QMetaType::Int, validateTimeSignatureNumerator},
                {P_Denominator, QMetaType::Int, validateTimeSignatureDenominator}
            };
            case EI_Track: return {
                {P_Name, QMetaType::QString},
                {P_ColorId, QMetaType::Int},
                {P_ControlGain, QMetaType::Double},
                {P_ControlPan, QMetaType::Double, validatePan},
                {P_ControlMute, QMetaType::Bool},
                {P_ControlRecord, QMetaType::Bool},
                {P_ControlSolo, QMetaType::Bool},
                {P_Height, QMetaType::Double, validateDoubleGreaterOrEqualZero}
            };
            case EI_WorkspaceInfo: return {
                {P_JsonObject, QMetaType::QJsonObject}
            };
            default: Q_UNREACHABLE();
        }
    }

    inline ModelStrategy::PropertySpec ModelStrategy::getPropertySpecFromEntityTypeAndPropertyType(Entity entityType, Property propertyType) {
        auto v = getEntityPropertySpecsFromEntityType(entityType);
        auto it = std::ranges::find_if(v, [propertyType](const auto &spec) { return spec.propertyType == propertyType; });
        if (it == v.end()) {
            Q_UNREACHABLE();
        }
        return *it;
    }

    inline bool ModelStrategy::isEntityTypeAndPropertyTypeCompatible(Entity entityType, Property propertyType) {
        return std::ranges::any_of(getEntityPropertySpecsFromEntityType(entityType), [propertyType](const auto &spec) { return spec.propertyType == propertyType; });
    }



}

#endif //DIFFSCOPE_DSPX_MODEL_MODELSTRATEGY_H
