#ifndef DIFFSCOPE_DSPX_MODEL_BASICMODELSTRATEGY_H
#define DIFFSCOPE_DSPX_MODEL_BASICMODELSTRATEGY_H

#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class BasicModelStrategy : public ModelStrategy {
        Q_OBJECT
    public:
        explicit BasicModelStrategy(QObject *parent = nullptr);
        ~BasicModelStrategy() override;

        Handle createEntity(Entity entityType) override;
        void destroyEntity(Handle entity) override;
        void insertIntoSequenceContainer(Handle sequenceContainerEntity, Handle entity) override;
        void insertIntoListContainer(Handle listContainerEntity, const QList<Handle> &entities, int index) override;
        void insertIntoMapContainer(Handle mapContainerEntity, Handle entity, const QString &key) override;
        Handle takeFromSequenceContainer(Handle sequenceContainerEntity, Handle entity) override;
        QList<Handle> takeFromListContainer(Handle listContainerEntity, const QList<int> &indexes) override;
        Handle takeFromMapContainer(Handle mapContainerEntity, const QString &key) override;
        void rotateListContainer(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex) override;
        void setEntityProperty(Handle entity, Property property, const QVariant &value) override;
        QVariant getEntityProperty(Handle entity, Property property) override;
        Handle getAssociatedSubEntity(Handle entity, Relationship relationship) override;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_BASICMODELSTRATEGY_H
