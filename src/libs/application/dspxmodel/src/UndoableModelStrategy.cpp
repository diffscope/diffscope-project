#include "UndoableModelStrategy.h"
#include "UndoableModelStrategy_p.h"

#include <QUndoStack>

#include <dspxmodel/private/SpliceHelper_p.h>

namespace dspx {

    //================================================================
    // Create/Destroy Commands
    //================================================================

    CreateEntityCommand::CreateEntityCommand(UndoableModelStrategy *strategy, BasicModelStrategy::Entity entityType,
                                             QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_entityType(entityType), m_object(nullptr) {
    }

    CreateEntityCommand::~CreateEntityCommand() {
        // Delete object if command is in undone state
        if (m_object && m_undone) {
            delete m_object.data();
        }
    }

    void CreateEntityCommand::undo() {
        // Detach object from strategy but keep it alive
        m_object->setParent(nullptr);
        m_undone = true;
        Q_EMIT m_strategy->destroyEntityNotified(entity());
    }

    void CreateEntityCommand::redo() {
        if (!m_object) {
            // First time: create the object
            m_object = BasicModelStrategyEntity::createByType(m_entityType, m_strategy);
        } else {
            // Subsequent times: re-parent the existing object
            m_object->setParent(m_strategy);
        }
        m_undone = false;
        Q_EMIT m_strategy->createEntityNotified(entity(), m_entityType);
    }

    Handle CreateEntityCommand::entity() const {
        return {reinterpret_cast<quintptr>(m_object.data())};
    }

    DestroyEntityCommand::DestroyEntityCommand(UndoableModelStrategy *strategy, Handle entity,
                                               QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_object(handle_cast<BasicModelStrategyEntity>(entity)) {
    }

    DestroyEntityCommand::~DestroyEntityCommand() {
        // Delete object if command is NOT in undone state (i.e., destroy was executed)
        if (m_object && !m_undone) {
            delete m_object.data();
        }
    }

    void DestroyEntityCommand::recursivelyCreate(BasicModelStrategyEntity *object) {
        object->setParent(m_strategy);
        Q_EMIT m_strategy->createEntityNotified({reinterpret_cast<quintptr>(object)}, object->type);

        if (auto seq = qobject_cast<BasicModelStrategySequenceContainerEntity *>(object)) {
            for (auto child : std::as_const(seq->sequence)) {
                recursivelyCreate(child);
            }
        } else if (auto list = qobject_cast<BasicModelStrategyListContainerEntity *>(object)) {
            for (auto child : std::as_const(list->list)) {
                recursivelyCreate(child);
            }
        } else if (auto map = qobject_cast<BasicModelStrategyMapContainerEntity *>(object)) {
            for (auto child : std::as_const(map->map)) {
                recursivelyCreate(child);
            }
        }
    }

    void DestroyEntityCommand::recursivelyDestroy(BasicModelStrategyEntity *object) {
        if (auto seq = qobject_cast<BasicModelStrategySequenceContainerEntity *>(object)) {
            for (auto child : std::as_const(seq->sequence)) {
                recursivelyDestroy(child);
            }
        } else if (auto list = qobject_cast<BasicModelStrategyListContainerEntity *>(object)) {
            for (auto child : std::as_const(list->list)) {
                recursivelyDestroy(child);
            }
        } else if (auto map = qobject_cast<BasicModelStrategyMapContainerEntity *>(object)) {
            for (auto child : std::as_const(map->map)) {
                recursivelyDestroy(child);
            }
        }

        object->setParent(nullptr);
        Q_EMIT m_strategy->destroyEntityNotified({reinterpret_cast<quintptr>(object)});
    }

    void DestroyEntityCommand::undo() {
        recursivelyCreate(m_object);
        m_undone = true;
    }

    void DestroyEntityCommand::redo() {
        recursivelyDestroy(m_object);
        m_undone = false;
    }

    //================================================================
    // Container Commands
    //================================================================

    InsertIntoSequenceContainerCommand::InsertIntoSequenceContainerCommand(
        UndoableModelStrategy *strategy, Handle sequenceContainerEntity,
        Handle entity, QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(sequenceContainerEntity), m_entity(entity) {
    }

    void InsertIntoSequenceContainerCommand::undo() {
        auto containerObj = handle_cast<BasicModelStrategySequenceContainerEntity>(m_container);
        auto entityObj = handle_cast<BasicModelStrategyEntity>(m_entity);
        containerObj->sequence.remove(entityObj);
        entityObj->setParent(m_strategy);
        Q_EMIT m_strategy->takeFromContainerNotified(m_entity, m_container, m_entity);
    }

    void InsertIntoSequenceContainerCommand::redo() {
        auto containerObj = handle_cast<BasicModelStrategySequenceContainerEntity>(m_container);
        auto entityObj = handle_cast<BasicModelStrategyEntity>(m_entity);
        containerObj->sequence.insert(entityObj);
        entityObj->setParent(containerObj);
        Q_EMIT m_strategy->insertIntoSequenceContainerNotified(m_container, m_entity);
    }

    TakeFromSequenceContainerCommand::TakeFromSequenceContainerCommand(
        UndoableModelStrategy *strategy, Handle sequenceContainerEntity,
        Handle entity, QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(sequenceContainerEntity),
          m_object(handle_cast<BasicModelStrategyEntity>(entity)) {
    }

    TakeFromSequenceContainerCommand::~TakeFromSequenceContainerCommand() {
        // Delete object if command is NOT in undone state (i.e., take was executed)
        if (m_object && !m_undone) {
            delete m_object.data();
        }
    }

    void TakeFromSequenceContainerCommand::undo() {
        auto containerObj = handle_cast<BasicModelStrategySequenceContainerEntity>(m_container);
        containerObj->sequence.insert(m_object);
        m_object->setParent(containerObj);
        m_undone = true;
        Q_EMIT m_strategy->insertIntoSequenceContainerNotified(m_container, {reinterpret_cast<quintptr>(m_object.data())});
    }

    void TakeFromSequenceContainerCommand::redo() {
        auto containerObj = handle_cast<BasicModelStrategySequenceContainerEntity>(m_container);
        containerObj->sequence.remove(m_object);
        m_object->setParent(m_strategy);
        m_undone = false;
        Q_EMIT m_strategy->takeFromContainerNotified({reinterpret_cast<quintptr>(m_object.data())}, m_container,
                                                     {reinterpret_cast<quintptr>(m_object.data())});
    }

    MoveToAnotherSequenceContainerCommand::MoveToAnotherSequenceContainerCommand(UndoableModelStrategy *strategy,
                                                                                 Handle sequenceContainerEntity,
                                                                                 Handle entity,
                                                                                 Handle otherSequenceContainerEntity,
                                                                                 QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(sequenceContainerEntity), m_entity(entity),
          m_otherContainer(otherSequenceContainerEntity) {
    }

    void MoveToAnotherSequenceContainerCommand::undo() {
        auto containerObj = handle_cast<BasicModelStrategySequenceContainerEntity>(m_otherContainer);
        auto otherContainerObj = handle_cast<BasicModelStrategySequenceContainerEntity>(m_container);
        auto entityObj = handle_cast<BasicModelStrategyEntity>(m_entity);
        containerObj->sequence.remove(entityObj);
        otherContainerObj->sequence.insert(entityObj);
        entityObj->setParent(otherContainerObj);
        Q_EMIT m_strategy->moveToAnotherSequenceContainerNotified(m_otherContainer, m_entity, m_container);
    }

    void MoveToAnotherSequenceContainerCommand::redo() {
        auto containerObj = handle_cast<BasicModelStrategySequenceContainerEntity>(m_container);
        auto otherContainerObj = handle_cast<BasicModelStrategySequenceContainerEntity>(m_otherContainer);
        auto entityObj = handle_cast<BasicModelStrategyEntity>(m_entity);
        containerObj->sequence.remove(entityObj);
        otherContainerObj->sequence.insert(entityObj);
        entityObj->setParent(otherContainerObj);
        Q_EMIT m_strategy->moveToAnotherSequenceContainerNotified(m_container, m_entity, m_otherContainer);
    }

    InsertIntoListContainerCommand::InsertIntoListContainerCommand(
        UndoableModelStrategy *strategy, Handle listContainerEntity,
        Handle entity, int index, QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(listContainerEntity), m_entity(entity),
          m_index(index) {
    }

    void InsertIntoListContainerCommand::undo() {
        auto containerObj = handle_cast<BasicModelStrategyListContainerEntity>(m_container);
        auto entityObj = handle_cast<BasicModelStrategyEntity>(m_entity);
        containerObj->list.removeAt(m_index);
        entityObj->setParent(m_strategy);
        Q_EMIT m_strategy->takeFromListContainerNotified(m_entity, m_container, m_index);
    }

    void InsertIntoListContainerCommand::redo() {
        auto containerObj = handle_cast<BasicModelStrategyListContainerEntity>(m_container);
        auto entityObj = handle_cast<BasicModelStrategyEntity>(m_entity);
        containerObj->list.insert(m_index, entityObj);
        entityObj->setParent(containerObj);
        Q_EMIT m_strategy->insertIntoListContainerNotified(m_container, m_entity, m_index);
    }

    TakeFromListContainerCommand::TakeFromListContainerCommand(UndoableModelStrategy *strategy,
                                                               Handle listContainerEntity,
                                                               int index, QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(listContainerEntity), m_object(nullptr),
          m_index(index) {
    }

    TakeFromListContainerCommand::~TakeFromListContainerCommand() {
        // Delete object if command is NOT in undone state (i.e., take was executed)
        if (m_object && !m_undone) {
            delete m_object.data();
        }
    }

    void TakeFromListContainerCommand::undo() {
        auto containerObj = handle_cast<BasicModelStrategyListContainerEntity>(m_container);
        containerObj->list.insert(m_index, m_object);
        m_object->setParent(containerObj);
        m_undone = true;
        Q_EMIT m_strategy->insertIntoListContainerNotified(m_container, entity(), m_index);
    }

    void TakeFromListContainerCommand::redo() {
        if (!m_object) {
            auto containerObj = handle_cast<BasicModelStrategyListContainerEntity>(m_container);
            m_object = containerObj->list.at(m_index);
        }
        auto containerObj = handle_cast<BasicModelStrategyListContainerEntity>(m_container);
        containerObj->list.removeAt(m_index);
        m_object->setParent(m_strategy);
        m_undone = false;
        Q_EMIT m_strategy->takeFromListContainerNotified(entity(), m_container, m_index);
    }

    Handle TakeFromListContainerCommand::entity() const {
        return {reinterpret_cast<quintptr>(m_object.data())};
    }

    InsertIntoMapContainerCommand::InsertIntoMapContainerCommand(UndoableModelStrategy *strategy,
                                                                 Handle mapContainerEntity,
                                                                 Handle entity, const QString &key,
                                                                 QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(mapContainerEntity), m_entity(entity), m_key(key),
          m_oldObject(nullptr) {
    }

    InsertIntoMapContainerCommand::~InsertIntoMapContainerCommand() {
        if (m_oldObject) {
            delete m_oldObject.data();
        }
    }

    void InsertIntoMapContainerCommand::undo() {
        auto containerObj = handle_cast<BasicModelStrategyMapContainerEntity>(m_container);
        auto entityObj = handle_cast<BasicModelStrategyEntity>(m_entity);

        containerObj->map.remove(m_key);
        entityObj->setParent(m_strategy);
        Q_EMIT m_strategy->takeFromMapContainerNotified(m_entity, m_container, m_key);

        if (m_oldObject) {
            containerObj->map.insert(m_key, m_oldObject);
            m_oldObject->setParent(containerObj);
            Q_EMIT m_strategy->insertIntoMapContainerNotified(m_container,
                                                              {reinterpret_cast<quintptr>(m_oldObject.data())}, m_key);
            m_oldObject = nullptr; // Transfer ownership back to container
        }
    }

    void InsertIntoMapContainerCommand::redo() {
        auto containerObj = handle_cast<BasicModelStrategyMapContainerEntity>(m_container);
        auto entityObj = handle_cast<BasicModelStrategyEntity>(m_entity);

        if (containerObj->map.contains(m_key)) {
            m_oldObject = containerObj->map.take(m_key);
            m_oldObject->setParent(nullptr); // Take ownership
            Q_EMIT m_strategy->takeFromMapContainerNotified({reinterpret_cast<quintptr>(m_oldObject.get())}, m_container,
                                                            m_key);
        }

        containerObj->map.insert(m_key, entityObj);
        entityObj->setParent(containerObj);
        Q_EMIT m_strategy->insertIntoMapContainerNotified(m_container, m_entity, m_key);
    }

    TakeFromMapContainerCommand::TakeFromMapContainerCommand(UndoableModelStrategy *strategy,
                                                             Handle mapContainerEntity,
                                                             const QString &key, QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(mapContainerEntity), m_object(nullptr), m_key(key) {
    }

    TakeFromMapContainerCommand::~TakeFromMapContainerCommand() {
        // Delete object if command is NOT in undone state (i.e., take was executed)
        if (m_object && !m_undone) {
            delete m_object.data();
        }
    }

    void TakeFromMapContainerCommand::undo() {
        auto containerObj = handle_cast<BasicModelStrategyMapContainerEntity>(m_container);
        containerObj->map.insert(m_key, m_object);
        m_object->setParent(containerObj);
        m_undone = true;
        Q_EMIT m_strategy->insertIntoMapContainerNotified(m_container, entity(), m_key);
    }

    void TakeFromMapContainerCommand::redo() {
        if (!m_object) {
            auto containerObj = handle_cast<BasicModelStrategyMapContainerEntity>(m_container);
            m_object = containerObj->map.value(m_key);
        }
        auto containerObj = handle_cast<BasicModelStrategyMapContainerEntity>(m_container);
        containerObj->map.remove(m_key);
        m_object->setParent(m_strategy);
        m_undone = false;
        Q_EMIT m_strategy->takeFromMapContainerNotified(entity(), m_container, m_key);
    }

    Handle TakeFromMapContainerCommand::entity() const {
        return {reinterpret_cast<quintptr>(m_object.data())};
    }

    RotateListContainerCommand::RotateListContainerCommand(UndoableModelStrategy *strategy,
                                                           Handle listContainerEntity, int left,
                                                           int middle, int right, QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(listContainerEntity), m_left(left),
          m_middle(middle), m_right(right) {
    }

    void RotateListContainerCommand::undo() {
        auto &list = handle_cast<BasicModelStrategyListContainerEntity>(m_container)->list;
        std::rotate(list.begin() + m_left, list.begin() + m_right - (m_middle - m_left), list.begin() + m_right);
        Q_EMIT m_strategy->rotateListContainerNotified(m_container, m_left, m_right - (m_middle - m_left), m_right);
    }

    void RotateListContainerCommand::redo() {
        auto &list = handle_cast<BasicModelStrategyListContainerEntity>(m_container)->list;
        std::rotate(list.begin() + m_left, list.begin() + m_middle, list.begin() + m_right);
        Q_EMIT m_strategy->rotateListContainerNotified(m_container, m_left, m_middle, m_right);
    }

    //================================================================
    // Property & Data Commands
    //================================================================

    SetEntityPropertyCommand::SetEntityPropertyCommand(UndoableModelStrategy *strategy,
                                                       Handle entity,
                                                       BasicModelStrategy::Property property, const QVariant &value,
                                                       QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_entity(entity), m_property(property), m_newValue(value) {
        m_oldValue = m_strategy->getEntityProperty(m_entity, m_property);
    }

    void SetEntityPropertyCommand::undo() {
        auto object = handle_cast<BasicModelStrategyItemEntity>(m_entity);
        object->properties.insert(m_property, m_oldValue);
        Q_EMIT m_strategy->setEntityPropertyNotified(m_entity, m_property, m_oldValue);
    }

    void SetEntityPropertyCommand::redo() {
        auto object = handle_cast<BasicModelStrategyItemEntity>(m_entity);
        object->properties.insert(m_property, m_newValue);
        Q_EMIT m_strategy->setEntityPropertyNotified(m_entity, m_property, m_newValue);
    }

    bool SetEntityPropertyCommand::mergeWith(const QUndoCommand *command) {
        const auto other = static_cast<const SetEntityPropertyCommand *>(command);
        if (other->m_entity != m_entity || other->m_property != m_property) {
            return false;
        }
        m_newValue = other->m_newValue;
        return true;
    }

    int SetEntityPropertyCommand::id() const {
        // Return a constant to force mergeWith to be called for comparison
        return 1;
    }

    SpliceDataArrayCommand::SpliceDataArrayCommand(UndoableModelStrategy *strategy,
                                                   Handle dataContainerEntity, int index,
                                                   int length, const QVariantList &values, QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(dataContainerEntity), m_index(index),
          m_length(length), m_values(values) {
    }

    void SpliceDataArrayCommand::undo() {
        auto &data = handle_cast<BasicModelStrategyDataArrayEntity>(m_container)->data;
        SpliceHelper::splice(data, data.begin() + m_index, data.begin() + m_index + m_values.size(),
                             m_oldValues.begin(), m_oldValues.end());
        Q_EMIT m_strategy->spliceDataArrayNotified(m_container, m_index, m_values.size(), m_oldValues);
    }

    void SpliceDataArrayCommand::redo() {
        auto &data = handle_cast<BasicModelStrategyDataArrayEntity>(m_container)->data;
        if (m_oldValues.isEmpty() && m_length > 0) {
            m_oldValues = data.mid(m_index, m_length);
        }
        SpliceHelper::splice(data, data.begin() + m_index, data.begin() + m_index + m_length, m_values.begin(),
                             m_values.end());
        Q_EMIT m_strategy->spliceDataArrayNotified(m_container, m_index, m_length, m_values);
    }

    RotateDataArrayCommand::RotateDataArrayCommand(UndoableModelStrategy *strategy,
                                                   Handle dataContainerEntity, int left,
                                                   int middle, int right, QUndoCommand *parent)
        : QUndoCommand(parent), m_strategy(strategy), m_container(dataContainerEntity), m_left(left),
          m_middle(middle), m_right(right) {
    }

    void RotateDataArrayCommand::undo() {
        auto &data = handle_cast<BasicModelStrategyDataArrayEntity>(m_container)->data;
        std::rotate(data.begin() + m_left, data.begin() + m_right - (m_middle - m_left), data.begin() + m_right);
        Q_EMIT m_strategy->rotateDataArrayNotified(m_container, m_left, m_right - (m_middle - m_left), m_right);
    }

    void RotateDataArrayCommand::redo() {
        auto &data = handle_cast<BasicModelStrategyDataArrayEntity>(m_container)->data;
        std::rotate(data.begin() + m_left, data.begin() + m_middle, data.begin() + m_right);
        Q_EMIT m_strategy->rotateDataArrayNotified(m_container, m_left, m_middle, m_right);
    }

    //================================================================
    // UndoableModelStrategy
    //================================================================

    UndoableModelStrategy::UndoableModelStrategy(QObject *parent)
        : BasicModelStrategy(parent), m_undoStack(new QUndoStack(this)) {
    }

    UndoableModelStrategy::~UndoableModelStrategy() = default;

    QUndoStack *UndoableModelStrategy::undoStack() const {
        return m_undoStack;
    }

    Handle UndoableModelStrategy::createEntity(Entity entityType) {
        auto cmd = new CreateEntityCommand(this, entityType);
        m_undoStack->push(cmd);
        return cmd->entity();
    }

    void UndoableModelStrategy::destroyEntity(Handle entity) {
        m_undoStack->push(new DestroyEntityCommand(this, entity));
    }

    bool UndoableModelStrategy::insertIntoSequenceContainer(Handle sequenceContainerEntity, Handle entity) {
        auto entityObj = handle_cast<BasicModelStrategyEntity>(entity);
        if (entityObj->parent() != this) {
            return false;
        }
        m_undoStack->push(new InsertIntoSequenceContainerCommand(this, sequenceContainerEntity, entity));
        return true;
    }

    bool UndoableModelStrategy::insertIntoListContainer(Handle listContainerEntity, Handle entity, int index) {
        auto listContainerObject = handle_cast<BasicModelStrategyListContainerEntity>(listContainerEntity);
        if (index < 0 || index > listContainerObject->list.size()) {
            return false;
        }
        auto entityObj = handle_cast<BasicModelStrategyEntity>(entity);
        if (entityObj->parent() != this) {
            return false;
        }
        m_undoStack->push(new InsertIntoListContainerCommand(this, listContainerEntity, entity, index));
        return true;
    }

    bool UndoableModelStrategy::insertIntoMapContainer(Handle mapContainerEntity, Handle entity, const QString &key) {
        auto entityObj = handle_cast<BasicModelStrategyEntity>(entity);
        if (entityObj->parent() != this) {
            return false;
        }
        m_undoStack->push(new InsertIntoMapContainerCommand(this, mapContainerEntity, entity, key));
        return true;
    }

    bool UndoableModelStrategy::moveToAnotherSequenceContainer(Handle sequenceContainerEntity, Handle entity,
                                                               Handle otherSequenceContainerEntity) {
        auto sequenceContainerObject = handle_cast<BasicModelStrategySequenceContainerEntity>(sequenceContainerEntity);
        auto otherSequenceContainerObject = handle_cast<BasicModelStrategySequenceContainerEntity>(otherSequenceContainerEntity);
        auto object = handle_cast<BasicModelStrategyEntity>(entity);
        if (sequenceContainerObject == otherSequenceContainerObject) {
            return false;
        }
        if (!sequenceContainerObject->sequence.contains(object)) {
            return false;
        }
        m_undoStack->push(new MoveToAnotherSequenceContainerCommand(this, sequenceContainerEntity, entity,
                                                                    otherSequenceContainerEntity));
        return true;
    }

    Handle UndoableModelStrategy::takeFromSequenceContainer(Handle sequenceContainerEntity,
                                                                                Handle entity) {
        auto sequenceContainerObject = handle_cast<BasicModelStrategySequenceContainerEntity>(sequenceContainerEntity);
        auto object = reinterpret_cast<BasicModelStrategyEntity *>(entity.d);
        if (!sequenceContainerObject->sequence.contains(object)) {
            return {};
        }
        auto cmd = new TakeFromSequenceContainerCommand(this, sequenceContainerEntity, entity);
        m_undoStack->push(cmd);
        return entity;
    }

    Handle UndoableModelStrategy::takeFromListContainer(Handle listContainerEntity, int index) {
        auto listContainerObject = handle_cast<BasicModelStrategyListContainerEntity>(listContainerEntity);
        if (index < 0 || index >= listContainerObject->list.size()) {
            return {};
        }
        auto cmd = new TakeFromListContainerCommand(this, listContainerEntity, index);
        m_undoStack->push(cmd);
        return cmd->entity();
    }

    Handle UndoableModelStrategy::takeFromMapContainer(Handle mapContainerEntity,
                                                                           const QString &key) {
        auto mapContainerObject = handle_cast<BasicModelStrategyMapContainerEntity>(mapContainerEntity);
        if (!mapContainerObject->map.contains(key)) {
            return {};
        }
        auto cmd = new TakeFromMapContainerCommand(this, mapContainerEntity, key);
        m_undoStack->push(cmd);
        return cmd->entity();
    }

    bool UndoableModelStrategy::rotateListContainer(Handle listContainerEntity, int leftIndex, int middleIndex,
                                                    int rightIndex) {
        auto listContainerObject = handle_cast<BasicModelStrategyListContainerEntity>(listContainerEntity);
        if (leftIndex < 0 || leftIndex > listContainerObject->list.size() || middleIndex < leftIndex ||
            middleIndex > listContainerObject->list.size() || rightIndex < middleIndex ||
            rightIndex > listContainerObject->list.size()) {
            return false;
        }
        m_undoStack->push(new RotateListContainerCommand(this, listContainerEntity, leftIndex, middleIndex, rightIndex));
        return true;
    }

    void UndoableModelStrategy::setEntityProperty(Handle entity, Property property, const QVariant &value) {
        auto object = handle_cast<BasicModelStrategyItemEntity>(entity);
        Q_ASSERT(isEntityTypeAndPropertyTypeCompatible(object->type, property));
        m_undoStack->push(new SetEntityPropertyCommand(this, entity, property, value));
    }

    bool UndoableModelStrategy::spliceDataArray(Handle dataArrayEntity, int index, int length,
                                                const QVariantList &values) {
        auto &data = handle_cast<BasicModelStrategyDataArrayEntity>(dataArrayEntity)->data;
        if (index < 0 || index > data.size() || length < 0 || index + length > data.size()) {
            return false;
        }
        m_undoStack->push(new SpliceDataArrayCommand(this, dataArrayEntity, index, length, values));
        return true;
    }

    bool UndoableModelStrategy::rotateDataArray(Handle dataArrayEntity, int leftIndex, int middleIndex,
                                                int rightIndex) {
        auto &data = handle_cast<BasicModelStrategyDataArrayEntity>(dataArrayEntity)->data;
        if (leftIndex < 0 || leftIndex > data.size() || middleIndex < leftIndex || middleIndex > data.size() ||
            rightIndex < middleIndex || rightIndex > data.size()) {
            return false;
        }
        m_undoStack->push(new RotateDataArrayCommand(this, dataArrayEntity, leftIndex, middleIndex, rightIndex));
        return true;
    }

}
