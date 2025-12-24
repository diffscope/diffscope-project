#ifndef DIFFSCOPE_DSPX_MODEL_UNDOABLEMODELSTRATEGY_H
#define DIFFSCOPE_DSPX_MODEL_UNDOABLEMODELSTRATEGY_H

#include <dspxmodel/BasicModelStrategy.h>

class QUndoStack;

namespace dspx {

    class DSPX_MODEL_EXPORT UndoableModelStrategy : public BasicModelStrategy {
        Q_OBJECT
    public:
        explicit UndoableModelStrategy(QObject *parent = nullptr);
        ~UndoableModelStrategy() override;

        QUndoStack *undoStack() const;

        Handle createEntity(Entity entityType) override;
        void destroyEntity(Handle entity) override;
        bool insertIntoSequenceContainer(Handle sequenceContainerEntity, Handle entity) override;
        bool insertIntoListContainer(Handle listContainerEntity, Handle entity, int index) override;
        bool insertIntoMapContainer(Handle mapContainerEntity, Handle entity, const QString &key) override;
        Handle takeFromSequenceContainer(Handle sequenceContainerEntity, Handle entity) override;
        Handle takeFromListContainer(Handle listContainerEntity, int index) override;
        Handle takeFromMapContainer(Handle mapContainerEntity, const QString &key) override;
        bool rotateListContainer(Handle listContainerEntity, int leftIndex, int middleIndex, int rightIndex) override;
        void setEntityProperty(Handle entity, Property property, const QVariant &value) override;
        bool spliceDataArray(Handle dataContainerEntity, int index, int length, const QVariantList &values) override;
        bool rotateDataArray(Handle dataContainerEntity, int leftIndex, int middleIndex, int rightIndex) override;

    private:
        QUndoStack *m_undoStack;
    };

}

#endif // DIFFSCOPE_DSPX_MODEL_UNDOABLEMODELSTRATEGY_H
