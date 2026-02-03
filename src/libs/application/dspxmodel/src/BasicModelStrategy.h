#ifndef DIFFSCOPE_DSPX_MODEL_BASICMODELSTRATEGY_H
#define DIFFSCOPE_DSPX_MODEL_BASICMODELSTRATEGY_H

#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class DSPX_MODEL_EXPORT BasicModelStrategy : public ModelStrategy {
        Q_OBJECT
    public:
        explicit BasicModelStrategy(QObject *parent = nullptr);
        ~BasicModelStrategy() override;

        Handle createEntity(Entity entityType) override;
        void destroyEntity(Handle entity) override;
        Entity getEntityType(Handle entity) override;
        QList<Handle> getEntitiesFromSequenceContainer(Handle sequenceContainerEntity) override;
        QList<Handle> getEntitiesFromListContainer(Handle listContainerEntity) override;
        QList<QPair<QString, Handle>> getEntitiesFromMapContainer(Handle mapContainerEntity) override;
        bool insertIntoSequenceContainer(Handle sequenceContainerEntity, Handle entity) override;
        bool insertIntoListContainer(Handle listContainerEntity, Handle entity, int index) override;
        bool insertIntoMapContainer(Handle mapContainerEntity, Handle entity, const QString &key) override;
        bool moveToAnotherSequenceContainer(Handle sequenceContainerEntity, Handle entity, Handle otherSequenceContainerEntity) override;
        Handle takeFromSequenceContainer(Handle sequenceContainerEntity, Handle entity) override;
        Handle takeFromListContainer(Handle listContainerEntity, int index) override;
        Handle takeFromMapContainer(Handle mapContainerEntity, const QString &key) override;
        bool rotateListContainer(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex) override;
        void setEntityProperty(Handle entity, Property property, const QVariant &value) override;
        QVariant getEntityProperty(Handle entity, Property property) override;
        bool spliceDataArray(Handle dataContainerEntity, int index, int length, const QVariantList &values) override;
        QVariantList sliceDataArray(Handle dataContainerEntity, int index, int length) override;
        int getSizeOfDataArray(Handle dataContainerEntity) override;
        bool rotateDataArray(Handle dataContainerEntity, int leftIndex, int middleIndex, int rightIndex) override;
        Handle getAssociatedSubEntity(Handle entity, Relationship relationship) override;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_BASICMODELSTRATEGY_H
