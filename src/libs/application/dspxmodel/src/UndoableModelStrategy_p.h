#ifndef DIFFSCOPE_DSPX_MODEL_UNDOABLEMODELSTRATEGY_P_H
#define DIFFSCOPE_DSPX_MODEL_UNDOABLEMODELSTRATEGY_P_H

#include <QUndoCommand>
#include <QPointer>

#include <dspxmodel/private/BasicModelStrategyEntity_p.h>
#include <dspxmodel/UndoableModelStrategy.h>

namespace dspx {

    // Helper to cast handle to internal object
    template <class T>
    inline T *handle_cast(Handle entity) {
        return reinterpret_cast<T *>(entity.d);
    }

    //================================================================
    // Create/Destroy Commands
    //================================================================

    class CreateEntityCommand : public QUndoCommand {
    public:
        CreateEntityCommand(UndoableModelStrategy *strategy, BasicModelStrategy::Entity entityType,
                            QUndoCommand *parent = nullptr);
        ~CreateEntityCommand() override;
        void undo() override;
        void redo() override;
        Handle entity() const;

    private:
        UndoableModelStrategy *m_strategy;
        BasicModelStrategy::Entity m_entityType;
        QPointer<BasicModelStrategyEntity> m_object;
        bool m_undone = false;
    };

    class DestroyEntityCommand : public QUndoCommand {
    public:
        DestroyEntityCommand(UndoableModelStrategy *strategy, Handle entity,
                             QUndoCommand *parent = nullptr);
        ~DestroyEntityCommand() override;
        void undo() override;
        void redo() override;

    private:
        UndoableModelStrategy *m_strategy;
        QPointer<BasicModelStrategyEntity> m_object;
        bool m_undone = false;
        void recursivelyDestroy(BasicModelStrategyEntity *object);
        void recursivelyCreate(BasicModelStrategyEntity *object);
    };

    //================================================================
    // Container Commands
    //================================================================

    class InsertIntoSequenceContainerCommand : public QUndoCommand {
    public:
        InsertIntoSequenceContainerCommand(UndoableModelStrategy *strategy,
                                           Handle sequenceContainerEntity,
                                           Handle entity, QUndoCommand *parent = nullptr);
        ~InsertIntoSequenceContainerCommand() override = default;
        void undo() override;
        void redo() override;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_container;
        Handle m_entity;
    };

    class TakeFromSequenceContainerCommand : public QUndoCommand {
    public:
        TakeFromSequenceContainerCommand(UndoableModelStrategy *strategy,
                                         Handle sequenceContainerEntity,
                                         Handle entity, QUndoCommand *parent = nullptr);
        ~TakeFromSequenceContainerCommand() override;
        void undo() override;
        void redo() override;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_container;
        QPointer<BasicModelStrategyEntity> m_object;
        bool m_undone = false;
    };

    class InsertIntoListContainerCommand : public QUndoCommand {
    public:
        InsertIntoListContainerCommand(UndoableModelStrategy *strategy, Handle listContainerEntity,
                                       Handle entity, int index, QUndoCommand *parent = nullptr);
        ~InsertIntoListContainerCommand() override = default;
        void undo() override;
        void redo() override;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_container;
        Handle m_entity;
        int m_index;
    };

    class TakeFromListContainerCommand : public QUndoCommand {
    public:
        TakeFromListContainerCommand(UndoableModelStrategy *strategy, Handle listContainerEntity,
                                     int index, QUndoCommand *parent = nullptr);
        ~TakeFromListContainerCommand() override;
        void undo() override;
        void redo() override;
        Handle entity() const;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_container;
        QPointer<BasicModelStrategyEntity> m_object;
        int m_index;
        bool m_undone = false;
    };

    class InsertIntoMapContainerCommand : public QUndoCommand {
    public:
        InsertIntoMapContainerCommand(UndoableModelStrategy *strategy, Handle mapContainerEntity,
                                      Handle entity, const QString &key,
                                      QUndoCommand *parent = nullptr);
        ~InsertIntoMapContainerCommand() override;
        void undo() override;
        void redo() override;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_container;
        Handle m_entity;
        QString m_key;
        QPointer<BasicModelStrategyEntity> m_oldObject;
    };

    class TakeFromMapContainerCommand : public QUndoCommand {
    public:
        TakeFromMapContainerCommand(UndoableModelStrategy *strategy, Handle mapContainerEntity,
                                    const QString &key, QUndoCommand *parent = nullptr);
        ~TakeFromMapContainerCommand() override;
        void undo() override;
        void redo() override;
        Handle entity() const;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_container;
        QPointer<BasicModelStrategyEntity> m_object;
        QString m_key;
        bool m_undone = false;
    };

    class RotateListContainerCommand : public QUndoCommand {
    public:
        RotateListContainerCommand(UndoableModelStrategy *strategy, Handle listContainerEntity,
                                   int left, int middle, int right, QUndoCommand *parent = nullptr);
        ~RotateListContainerCommand() override = default;
        void undo() override;
        void redo() override;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_container;
        int m_left, m_middle, m_right;
    };

    //================================================================
    // Property & Data Commands
    //================================================================

    class SetEntityPropertyCommand : public QUndoCommand {
    public:
        SetEntityPropertyCommand(UndoableModelStrategy *strategy, Handle entity,
                                 BasicModelStrategy::Property property, const QVariant &value,
                                 QUndoCommand *parent = nullptr);
        ~SetEntityPropertyCommand() override = default;
        void undo() override;
        void redo() override;
        bool mergeWith(const QUndoCommand *command) override;
        int id() const override;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_entity;
        BasicModelStrategy::Property m_property;
        QVariant m_newValue;
        QVariant m_oldValue;
    };

    class SpliceDataArrayCommand : public QUndoCommand {
    public:
        SpliceDataArrayCommand(UndoableModelStrategy *strategy, Handle dataContainerEntity,
                               int index, int length, const QVariantList &values, QUndoCommand *parent = nullptr);
        ~SpliceDataArrayCommand() override = default;
        void undo() override;
        void redo() override;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_container;
        int m_index;
        int m_length;
        QVariantList m_values;
        QVariantList m_oldValues;
    };

    class RotateDataArrayCommand : public QUndoCommand {
    public:
        RotateDataArrayCommand(UndoableModelStrategy *strategy, Handle dataContainerEntity,
                               int left, int middle, int right, QUndoCommand *parent = nullptr);
        ~RotateDataArrayCommand() override = default;
        void undo() override;
        void redo() override;

    private:
        UndoableModelStrategy *m_strategy;
        Handle m_container;
        int m_left, m_middle, m_right;
    };

}

#endif // DIFFSCOPE_DSPX_MODEL_UNDOABLEMODELSTRATEGY_P_H
