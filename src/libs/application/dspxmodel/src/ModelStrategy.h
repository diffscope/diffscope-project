#ifndef DIFFSCOPE_DSPX_MODEL_MODELSTRATEGY_H
#define DIFFSCOPE_DSPX_MODEL_MODELSTRATEGY_H

#include <QObject>

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

            ED_ParamCurveFreeItems,
            ED_VibratoPoints,

            EM_Params,
            EM_Sources,
            EM_Workspace,

        };

        enum Property {
            P_Author,
            P_CentShift,
            P_ClipStart,
            P_Color,
            P_ControlGain,
            P_ControlMute,
            P_ControlPan,
            P_ControlSolo,
            P_Denominator,
            P_EditorId,
            P_EditorName,
            P_JsonObject,
            P_KeyNumber,
            P_Language,
            P_Length,
            P_Measure,
            P_Name,
            P_Numerator,
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
            R_ParamCurvesEnvelop,
            R_ParamCurvesOriginal,
            R_Params,
            R_PhonemesEdited,
            R_PhonemesOriginal,
            R_Sources,
            R_Tempos,
            R_TimeSignatures,
            R_Tracks,
            R_VibratoPoints,
            R_Workspace,
        };

        virtual Handle createEntity(Entity entityType) = 0;
        virtual void destroyEntity(Handle entity) = 0;

        virtual void insertIntoSequenceContainer(Handle sequenceContainerEntity, Handle entity) = 0;
        virtual void insertIntoListContainer(Handle listContainerEntity, const QList<Handle> &entities, int index) = 0;
        virtual void insertIntoMapContainer(Handle mapContainerEntity, Handle entity, const QString &key) = 0;

        virtual void removeFromContainer(Handle entity) = 0;
        virtual QList<Handle> takeFromListContainer(Handle listContainerEntity, const QList<int> &indexes) = 0;
        virtual Handle takeFromMapContainer(Handle mapContainerEntity, const QString &key) = 0;
        virtual void rotateListContainer(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex) = 0;

        virtual void setEntityProperty(Handle entity, Property property, const QVariant &value) = 0;
        virtual QVariant getEntityProperty(Handle entity, Property property) = 0;

        virtual Handle getAssociatedSubEntity(Handle entity, Relationship relationship) = 0;

    Q_SIGNALS:
        void createEntityNotified(Handle entity, Entity entityType);
        void destroyEntityNotified(Handle entity);

        void insertIntoSequenceContainerNotified(Handle sequenceContainerEntity, Handle entity);
        void insertIntoListContainerNotified(Handle listContainerEntity, const QList<Handle> &entities, int index);
        void insertIntoMapContainerNotified(Handle mapContainerEntity, Handle entity, const QString &key);

        void removeFromContainerNotified(Handle entity);
        void takeFromListContainerNotified(const QList<Handle> &takenEntities, Handle listContainerEntity, const QList<int> &indexes);
        void takeFromMapContainerNotified(Handle takenEntity, Handle mapContainerEntity, const QString &key);
        void rotateListContainerNotified(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex);

        void setEntityPropertyNotified(Handle entity, Property property, const QVariant &value);


    };

}

#endif //DIFFSCOPE_DSPX_MODEL_MODELSTRATEGY_H
