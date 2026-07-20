#include <visualeditor/ParameterEditorContext.h>
#include <visualeditor/private/ParameterEditorContext_p.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include <QLoggingCategory>
#include <QState>
#include <QStateMachine>

#include <ScopicFlowCore/AnchorParameterViewModel.h>
#include <ScopicFlowCore/FreeParameterViewModel.h>
#include <ScopicFlowCore/ParameterEditorInteractionController.h>
#include <ScopicFlowCore/ParameterRangeSelectionViewModel.h>
#include <ScopicFlowCore/SelectionController.h>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/FreeParameterSelectionModel.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/SingerRegistry.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcParameterEditorContext, "diffscope.visualeditor.parametereditorcontext")

    namespace {

        int boundedRawValue(const Core::ParameterInfo &info, int value) {
            return std::clamp(value, qMin(info.bottomValue, info.topValue),
                              qMax(info.bottomValue, info.topValue));
        }

        void disconnectAll(QVector<QMetaObject::Connection> &connections) {
            for (const auto &connection : std::as_const(connections))
                QObject::disconnect(connection);
            connections.clear();
        }

        const char *operationName(ParameterViewModelBindingPrivate::Operation operation) {
            switch (operation) {
                case ParameterViewModelBindingPrivate::FreeEditing:
                    return "free editing";
                case ParameterViewModelBindingPrivate::AnchorInsertion:
                    return "anchor insertion";
                case ParameterViewModelBindingPrivate::AnchorMoving:
                    return "anchor moving";
                case ParameterViewModelBindingPrivate::AnchorDeletion:
                    return "anchor deletion";
                case ParameterViewModelBindingPrivate::AnchorConversion:
                    return "anchor conversion";
                case ParameterViewModelBindingPrivate::NoOperation:
                default:
                    return "none";
            }
        }

    }

    class AnchorParameterSelectionController final : public sflow::SelectionController {
    public:
        explicit AnchorParameterSelectionController(ParameterViewModelBindingPrivate *binding,
                                                    bool transform,
                                                    QObject *parent = nullptr)
            : SelectionController(parent), m_binding(binding), m_transform(transform) {
        }

        QObjectList getSelectedItems() const override {
            QObjectList result;
            if (!m_binding->selectionModel)
                return result;
            const auto &viewItems = m_transform ? m_binding->transformAnchorViewItems
                                                : m_binding->anchorViewItems;
            auto *viewModel = m_transform ? m_binding->anchorTransform : m_binding->anchorEdited;
            for (auto *item : m_binding->selectionModel->anchorNodeSelectionModel()->selectedItems()) {
                if (auto *viewItem = viewItems.value(item))
                    result.append(viewItem);
            }
            for (auto *viewItem : m_binding->pendingSelectedAnchors) {
                if (viewItem->anchorParameterViewModel() == viewModel && !result.contains(viewItem))
                    result.append(viewItem);
            }
            return result;
        }

        QObjectList getItemsBetween(QObject *startItem, QObject *endItem) const override {
            auto *start = qobject_cast<sflow::ParameterAnchorViewModel *>(startItem);
            auto *end = qobject_cast<sflow::ParameterAnchorViewModel *>(endItem);
            auto *viewModel = m_transform ? m_binding->anchorTransform : m_binding->anchorEdited;
            if (!start || !end || !viewModel)
                return {};
            const int first = qMin(start->position(), end->position());
            const int last = qMax(start->position(), end->position());
            return viewModel->slice(first, last - first + 1);
        }

        void select(QObject *item, SelectionCommand command) override {
            if (!m_binding->selectionModel)
                return;
            auto *viewItem = qobject_cast<sflow::ParameterAnchorViewModel *>(item);
            const auto documentCommand = dspx::SelectionModel::SelectionCommand(int(command));
            auto *container = m_binding->parameter
                ? (m_transform ? m_binding->parameter->anchorTransform()
                               : m_binding->parameter->anchorEdited())
                : nullptr;
            auto &documentItems = m_transform ? m_binding->transformAnchorDocumentItems
                                              : m_binding->anchorDocumentItems;

            if (command.testFlag(ClearPreviousSelection)) {
                for (auto *pending : std::as_const(m_binding->pendingSelectedAnchors))
                    pending->setSelected(false);
                m_binding->pendingSelectedAnchors.clear();
                if (viewItem && !documentItems.contains(viewItem)) {
                    m_binding->selectionModel->select(nullptr,
                        dspx::SelectionModel::ClearPreviousSelection,
                        dspx::SelectionModel::ST_AnchorNode, container);
                }
            }

            if (!viewItem) {
                if (command.testFlag(SetCurrentItem))
                    m_binding->pendingCurrentAnchor = nullptr;
                m_binding->selectionModel->select(nullptr, documentCommand,
                                                  dspx::SelectionModel::ST_AnchorNode, container);
                return;
            }

            if (auto *documentItem = documentItems.value(viewItem)) {
                if (command.testFlag(SetCurrentItem))
                    m_binding->pendingCurrentAnchor = nullptr;
                m_binding->selectionModel->select(documentItem, documentCommand,
                                                  dspx::SelectionModel::ST_AnchorNode, container);
                return;
            }

            if (command.testFlag(SetCurrentItem)) {
                m_binding->pendingCurrentAnchor = viewItem;
                Q_EMIT currentItemChanged();
            }

            const bool toggle = command.testFlag(Select) && command.testFlag(Deselect);
            const bool shouldSelect = toggle
                ? !m_binding->pendingSelectedAnchors.contains(viewItem)
                : command.testFlag(Select);
            if (!shouldSelect) {
                m_binding->pendingSelectedAnchors.remove(viewItem);
                viewItem->setSelected(false);
            } else {
                m_binding->pendingSelectedAnchors.insert(viewItem);
                viewItem->setSelected(true);
            }
        }

        QObject *currentItem() const override {
            if (!m_binding->selectionModel)
                return nullptr;
            auto *viewModel = m_transform ? m_binding->anchorTransform : m_binding->anchorEdited;
            if (m_binding->pendingCurrentAnchor &&
                m_binding->pendingCurrentAnchor->anchorParameterViewModel() == viewModel) {
                return m_binding->pendingCurrentAnchor;
            }
            const auto &viewItems = m_transform ? m_binding->transformAnchorViewItems
                                                : m_binding->anchorViewItems;
            return viewItems.value(
                m_binding->selectionModel->anchorNodeSelectionModel()->currentItem());
        }

        bool editScopeFocused() const override {
            if (!m_binding->selectionModel || !m_binding->parameter)
                return false;
            auto *anchorSelection = m_binding->selectionModel->anchorNodeSelectionModel();
            auto *container = m_transform ? m_binding->parameter->anchorTransform()
                                          : m_binding->parameter->anchorEdited();
            return m_binding->selectionModel->selectionType() == dspx::SelectionModel::ST_AnchorNode &&
                   anchorSelection->anchorNodeSequenceWithSelectedItems() == container;
        }

        void notifyCurrentItemChanged() { Q_EMIT currentItemChanged(); }
        void notifyEditScopeFocusedChanged() { Q_EMIT editScopeFocusedChanged(); }

    private:
        ParameterViewModelBindingPrivate *m_binding;
        bool m_transform{};
    };

    ParameterDefinitionListModel::ParameterDefinitionListModel(QObject *parent)
        : QAbstractListModel(parent) {
    }

    int ParameterDefinitionListModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_entries.size();
    }

    QVariant ParameterDefinitionListModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
            return {};
        const auto &entry = m_entries.at(index.row());
        switch (role) {
            case Qt::DisplayRole:
            case DisplayNameRole:
                return entry.displayName;
            case ParameterIdRole:
                return entry.parameterId;
            case RegisteredRole:
                return entry.registered;
            case WarningRole:
                return !entry.none && !entry.registered;
            case NoneRole:
                return entry.none;
            case ParameterInfoRole:
                return QVariant::fromValue(entry.parameterInfo);
            default:
                return {};
        }
    }

    QHash<int, QByteArray> ParameterDefinitionListModel::roleNames() const {
        return {
            {ParameterIdRole, "parameterId"},
            {DisplayNameRole, "displayName"},
            {RegisteredRole, "registered"},
            {WarningRole, "warning"},
            {NoneRole, "isNone"},
            {ParameterInfoRole, "parameterInfo"},
        };
    }

    void ParameterDefinitionListModel::setEntries(QList<Entry> entries) {
        beginResetModel();
        m_entries = std::move(entries);
        endResetModel();
    }

    const ParameterDefinitionListModel::Entry *ParameterDefinitionListModel::find(const QString &parameterId) const {
        const auto it = std::find_if(m_entries.cbegin(), m_entries.cend(), [&parameterId](const Entry &entry) {
            return entry.parameterId == parameterId;
        });
        return it == m_entries.cend() ? nullptr : &*it;
    }

    const ParameterDefinitionListModel::Entry *ParameterDefinitionListModel::firstAvailable() const {
        const auto it = std::find_if(m_entries.cbegin(), m_entries.cend(), [](const Entry &entry) {
            return !entry.none && entry.registered;
        });
        return it == m_entries.cend() ? nullptr : &*it;
    }

    ParameterViewModelBindingPrivate::ParameterViewModelBindingPrivate(ParameterViewModelBinding *q,
                                                                       bool editable_)
        : QObject(), q_ptr(q), editable(editable_) {
    }

    void ParameterViewModelBindingPrivate::initialize(Core::DspxDocument *document_) {
        document = document_;
        model = document->model();
        selectionModel = document->selectionModel();
        freeSelectionModel = document->freeParameterSelectionModel();

        original = new sflow::FreeParameterViewModel(q_ptr);
        freeEdited = new sflow::FreeParameterViewModel(q_ptr);
        freeTransform = new sflow::FreeParameterViewModel(q_ptr);
        anchorEdited = new sflow::AnchorParameterViewModel(q_ptr);
        anchorTransform = new sflow::AnchorParameterViewModel(q_ptr);
        freeSelection = new sflow::ParameterRangeSelectionViewModel(q_ptr);
        transformFreeSelection = new sflow::ParameterRangeSelectionViewModel(q_ptr);
        anchorSelectionController = new AnchorParameterSelectionController(this, false, q_ptr);
        transformAnchorSelectionController = new AnchorParameterSelectionController(this, true, q_ptr);
        interactionController = new sflow::ParameterEditorInteractionController(q_ptr);
        transformInteractionController = new sflow::ParameterEditorInteractionController(q_ptr);

        initializeStateMachine();
        initializeController();

        if (editable) {
            connect(freeEdited, &sflow::FreeParameterViewModel::spliced, this,
                [this](int index, int removeCount, const QList<QVariant> &values) {
                    if (updatingView)
                        return;
                    const bool progressing = currentOperation == FreeEditing &&
                        !currentOperationTransforms;
                    if (!progressing) {
                        updatingView = true;
                        if (parameter) {
                            auto *array = parameter->freeEdited();
                            freeEdited->splice(index, values.size(),
                                normalizeValues(array->slice(index, removeCount)));
                        } else {
                            freeEdited->splice(0, freeEdited->size(), {});
                        }
                        updatingView = false;
                        return;
                    }
                    freeDirtyFirst = qMin(freeDirtyFirst, index);
                    freeDirtyLast = qMax(freeDirtyLast,
                        index + qMax(removeCount, values.size()));
                });
            connect(freeTransform, &sflow::FreeParameterViewModel::spliced, this,
                [this](int index, int removeCount, const QList<QVariant> &values) {
                    if (updatingView)
                        return;
                    const bool progressing = currentOperation == FreeEditing &&
                        currentOperationTransforms;
                    if (!progressing) {
                        updatingView = true;
                        if (parameter) {
                            auto *array = parameter->freeTransform();
                            freeTransform->splice(index, values.size(),
                                normalizeValues(array->slice(index, removeCount), true));
                        } else {
                            freeTransform->splice(0, freeTransform->size(), {});
                        }
                        updatingView = false;
                        return;
                    }
                    freeDirtyFirst = qMin(freeDirtyFirst, index);
                    freeDirtyLast = qMax(freeDirtyLast,
                        index + qMax(removeCount, values.size()));
                });
            connect(anchorEdited, &sflow::AnchorParameterViewModel::itemUpdated, this,
                [this](sflow::ParameterAnchorViewModel *item) {
                    if (updatingView)
                        return;
                    const bool progressing = currentOperation != NoOperation;
                    if (progressing && !currentOperationTransforms) {
                        dirtyAnchors.insert(item);
                    } else if (auto *documentItem = anchorDocumentItems.value(item)) {
                        updatingView = true;
                        item->setPosition(documentItem->x());
                        item->setValue(parameterInfo.invokeNormalize(documentItem->y()));
                        item->setInterpolationMode(toViewInterpolation(documentItem->interpolationMode()));
                        updatingView = false;
                    }
                });
            connect(anchorTransform, &sflow::AnchorParameterViewModel::itemUpdated, this,
                [this](sflow::ParameterAnchorViewModel *item) {
                    if (updatingView)
                        return;
                    const bool progressing = currentOperation != NoOperation && currentOperationTransforms;
                    if (progressing) {
                        dirtyAnchors.insert(item);
                    } else if (auto *documentItem = transformAnchorDocumentItems.value(item)) {
                        const auto info = Core::transformParameterInfo();
                        updatingView = true;
                        item->setPosition(documentItem->x());
                        item->setValue(info.invokeNormalize(documentItem->y()));
                        item->setInterpolationMode(toViewInterpolation(documentItem->interpolationMode()));
                        updatingView = false;
                    }
                });
            connect(anchorEdited, &sflow::AnchorParameterViewModel::itemInserted, this,
                [this](sflow::ParameterAnchorViewModel *item) {
                    removedAnchorViews.remove(item);
                    if (!updatingView && !anchorDocumentItems.contains(item))
                        insertedAnchors.insert(item);
                });
            connect(anchorTransform, &sflow::AnchorParameterViewModel::itemInserted, this,
                [this](sflow::ParameterAnchorViewModel *item) {
                    removedAnchorViews.remove(item);
                    if (!updatingView && !transformAnchorDocumentItems.contains(item))
                        insertedAnchors.insert(item);
                });
            connect(anchorEdited, &sflow::AnchorParameterViewModel::itemRemoved, this,
                [this](sflow::ParameterAnchorViewModel *item) {
                    if (updatingView)
                        return;
                    removedAnchorViews.insert(item);
                    if (auto *documentItem = anchorDocumentItems.take(item)) {
                        anchorViewItems.remove(documentItem);
                        removedAnchors.insert(documentItem);
                    } else {
                        insertedAnchors.remove(item);
                    }
                    dirtyAnchors.remove(item);
                    pendingSelectedAnchors.remove(item);
                    if (pendingCurrentAnchor == item)
                        pendingCurrentAnchor = nullptr;
                });
            connect(anchorTransform, &sflow::AnchorParameterViewModel::itemRemoved, this,
                [this](sflow::ParameterAnchorViewModel *item) {
                    if (updatingView)
                        return;
                    removedAnchorViews.insert(item);
                    if (auto *documentItem = transformAnchorDocumentItems.take(item)) {
                        transformAnchorViewItems.remove(documentItem);
                        removedAnchors.insert(documentItem);
                    } else {
                        insertedAnchors.remove(item);
                    }
                    dirtyAnchors.remove(item);
                    pendingSelectedAnchors.remove(item);
                    if (pendingCurrentAnchor == item)
                        pendingCurrentAnchor = nullptr;
                });
        }

        auto *anchorSelection = selectionModel->anchorNodeSelectionModel();
        connect(anchorSelection, &dspx::AnchorNodeSelectionModel::itemSelected, this,
                [this](dspx::AnchorNode *item, bool selected) {
            if (auto *viewItem = anchorViewItems.value(item)) {
                updatingSelection = true;
                viewItem->setSelected(selected);
                updatingSelection = false;
            }
            if (auto *viewItem = transformAnchorViewItems.value(item)) {
                updatingSelection = true;
                viewItem->setSelected(selected);
                updatingSelection = false;
            }
        });
        connect(anchorSelection, &dspx::AnchorNodeSelectionModel::currentItemChanged,
                anchorSelectionController, &AnchorParameterSelectionController::notifyCurrentItemChanged);
        connect(anchorSelection, &dspx::AnchorNodeSelectionModel::anchorNodeSequenceWithSelectedItemsChanged,
                anchorSelectionController, &AnchorParameterSelectionController::notifyEditScopeFocusedChanged);
        connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged,
                anchorSelectionController, &AnchorParameterSelectionController::notifyEditScopeFocusedChanged);
        connect(anchorSelection, &dspx::AnchorNodeSelectionModel::currentItemChanged,
                transformAnchorSelectionController, &AnchorParameterSelectionController::notifyCurrentItemChanged);
        connect(anchorSelection, &dspx::AnchorNodeSelectionModel::anchorNodeSequenceWithSelectedItemsChanged,
                transformAnchorSelectionController, &AnchorParameterSelectionController::notifyEditScopeFocusedChanged);
        connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged,
                transformAnchorSelectionController, &AnchorParameterSelectionController::notifyEditScopeFocusedChanged);

        connect(freeSelectionModel, &Core::FreeParameterSelectionModel::rangeChanged, this, [this] {
            if (!editable || freeSelectionModel->singingClip() != singingClip ||
                freeSelectionModel->parameterId() != parameterId)
                return;
            updatingSelection = true;
            if (freeSelectionModel->layer() == Core::FreeParameterSelectionModel::TransformLayer) {
                freeSelection->clear();
                if (freeSelectionModel->hasSelection())
                    transformFreeSelection->setRange(freeSelectionModel->start(), freeSelectionModel->end());
                else
                    transformFreeSelection->clear();
            } else {
                transformFreeSelection->clear();
                if (freeSelectionModel->hasSelection())
                    freeSelection->setRange(freeSelectionModel->start(), freeSelectionModel->end());
                else
                    freeSelection->clear();
            }
            updatingSelection = false;
        });
    }

    void ParameterViewModelBindingPrivate::initializeStateMachine() {
        stateMachine = new QStateMachine(QState::ExclusiveStates, this);
        idleState = new QState;
        stateMachine->addState(idleState);
        stateMachine->setInitialState(idleState);

        for (auto &states : operationStates) {
            states.pending = new QState;
            states.progressing = new QState;
            states.committing = new QState;
            states.aborting = new QState;
            stateMachine->addState(states.pending);
            stateMachine->addState(states.progressing);
            stateMachine->addState(states.committing);
            stateMachine->addState(states.aborting);
            states.committing->addTransition(idleState);
            states.aborting->addTransition(idleState);
        }

#define BIND_OPERATION_TRANSITIONS(Index, Prefix) \
        idleState->addTransition(this, &ParameterViewModelBindingPrivate::Prefix##WillStart, operationStates[Index].pending); \
        operationStates[Index].pending->addTransition(this, &ParameterViewModelBindingPrivate::Prefix##Started, operationStates[Index].progressing); \
        operationStates[Index].pending->addTransition(this, &ParameterViewModelBindingPrivate::Prefix##NotStarted, idleState); \
        operationStates[Index].progressing->addTransition(this, &ParameterViewModelBindingPrivate::Prefix##WillCommit, operationStates[Index].committing); \
        operationStates[Index].progressing->addTransition(this, &ParameterViewModelBindingPrivate::Prefix##WillAbort, operationStates[Index].aborting)
        BIND_OPERATION_TRANSITIONS(0, free);
        BIND_OPERATION_TRANSITIONS(1, insertion);
        BIND_OPERATION_TRANSITIONS(2, moving);
        BIND_OPERATION_TRANSITIONS(3, deletion);
        BIND_OPERATION_TRANSITIONS(4, conversion);
#undef BIND_OPERATION_TRANSITIONS

        connect(idleState, &QState::entered, this, [this] {
            qCInfo(lcParameterEditorContext) << parameterId << "idle state entered";
            currentOperation = NoOperation;
            currentOperationTransforms = false;
        });
        connect(idleState, &QState::exited, this, [this] {
            qCInfo(lcParameterEditorContext) << parameterId << "idle state exited";
        });
        for (int i = 0; i < int(operationStates.size()); ++i) {
            const auto operation = Operation(i + 1);
            auto &states = operationStates[size_t(i)];
            connect(states.pending, &QState::entered, this, [this, operation] {
                qCInfo(lcParameterEditorContext) << parameterId << operationName(operation) << "pending state entered";
                enterPending(operation);
            });
            connect(states.pending, &QState::exited, this, [this, operation] {
                qCInfo(lcParameterEditorContext) << parameterId << operationName(operation) << "pending state exited";
            });
            connect(states.progressing, &QState::entered, this, [this, operation] {
                qCInfo(lcParameterEditorContext) << parameterId << operationName(operation) << "progressing state entered";
            });
            connect(states.progressing, &QState::exited, this, [this, operation] {
                qCInfo(lcParameterEditorContext) << parameterId << operationName(operation) << "progressing state exited";
            });
            connect(states.committing, &QState::entered, this, [this, operation] {
                qCInfo(lcParameterEditorContext) << parameterId << operationName(operation) << "committing state entered";
                enterCommitting(operation);
            });
            connect(states.committing, &QState::exited, this, [this, operation] {
                qCInfo(lcParameterEditorContext) << parameterId << operationName(operation) << "committing state exited";
            });
            connect(states.aborting, &QState::entered, this, [this, operation] {
                qCInfo(lcParameterEditorContext) << parameterId << operationName(operation) << "aborting state entered";
                enterAborting(operation);
            });
            connect(states.aborting, &QState::exited, this, [this, operation] {
                qCInfo(lcParameterEditorContext) << parameterId << operationName(operation) << "aborting state exited";
            });
        }
        stateMachine->start();
    }

    void ParameterViewModelBindingPrivate::initializeController() {
        if (!editable)
            return;
        const auto connectController = [this](sflow::ParameterEditorInteractionController *controller,
                                              bool transform) {
            connect(controller, &sflow::ParameterEditorInteractionController::freeEditingStarted,
                    this, [this, transform] { startOperation(FreeEditing, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::freeEditingCommitted,
                    this, [this, transform] { commitOperation(FreeEditing, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::freeEditingAborted,
                    this, [this, transform] { abortOperation(FreeEditing, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorInsertionStarted,
                    this, [this, transform] { startOperation(AnchorInsertion, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorInsertionCommitted,
                    this, [this, transform] { commitOperation(AnchorInsertion, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorInsertionAborted,
                    this, [this, transform] { abortOperation(AnchorInsertion, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorMovingStarted,
                    this, [this, transform] { startOperation(AnchorMoving, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorMovingCommitted,
                    this, [this, transform] { commitOperation(AnchorMoving, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorMovingAborted,
                    this, [this, transform] { abortOperation(AnchorMoving, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorDeletionStarted,
                    this, [this, transform] { startOperation(AnchorDeletion, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorDeletionCommitted,
                    this, [this, transform] { commitOperation(AnchorDeletion, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorDeletionAborted,
                    this, [this, transform] { abortOperation(AnchorDeletion, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorInterpolationChangingStarted,
                    this, [this, transform] { startOperation(AnchorConversion, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorInterpolationChangingCommitted,
                    this, [this, transform] { commitOperation(AnchorConversion, transform); });
            connect(controller, &sflow::ParameterEditorInteractionController::anchorInterpolationChangingAborted,
                    this, [this, transform] { abortOperation(AnchorConversion, transform); });

            connect(controller, &sflow::ParameterEditorInteractionController::freeRangeSelectingStarted,
                    this, [this, transform] {
                if (!singingClip || parameterId.isEmpty())
                    return;
                const auto name = parameterInfo.displayName.isEmpty() ? parameterId : parameterInfo.displayName;
                freeSelectionModel->setContext(singingClip, parameterId,
                    transform ? tr("%1 Transform").arg(name) : name,
                    transform ? Core::FreeParameterSelectionModel::TransformLayer
                              : Core::FreeParameterSelectionModel::EditedLayer);
            });
            connect(controller, &sflow::ParameterEditorInteractionController::freeRangeSelectingCommitted,
                    this, [this, transform](QQuickItem *, int start, int end) {
                if (!singingClip || parameterId.isEmpty())
                    return;
                const auto name = parameterInfo.displayName.isEmpty() ? parameterId : parameterInfo.displayName;
                freeSelectionModel->setContext(singingClip, parameterId,
                    transform ? tr("%1 Transform").arg(name) : name,
                    transform ? Core::FreeParameterSelectionModel::TransformLayer
                              : Core::FreeParameterSelectionModel::EditedLayer);
                freeSelectionModel->setRange(start, end);
            });
            connect(controller, &sflow::ParameterEditorInteractionController::freeRangeSelectingAborted,
                    this, [this, transform] {
                if (freeSelectionModel->singingClip() != singingClip ||
                    freeSelectionModel->parameterId() != parameterId ||
                    freeSelectionModel->layer() != (transform
                        ? Core::FreeParameterSelectionModel::TransformLayer
                        : Core::FreeParameterSelectionModel::EditedLayer)) {
                    return;
                }
                auto *viewSelection = transform ? transformFreeSelection : freeSelection;
                if (freeSelectionModel->hasSelection())
                    viewSelection->setRange(freeSelectionModel->start(), freeSelectionModel->end());
                else
                    viewSelection->clear();
            });
        };
        connectController(interactionController, false);
        connectController(transformInteractionController, true);
    }

    void ParameterViewModelBindingPrivate::setTarget(dspx::SingingClip *clip, const QString &id,
                                                     const Core::ParameterInfo &info, bool isRegistered) {
        const bool targetChanged = singingClip != clip || parameterId != id || registered != isRegistered;
        const bool infoChanged = parameterInfo != info;
        if (!targetChanged && !infoChanged)
            return;

        abortActiveEdit();
        disconnectAll(targetConnections);
        unbindParameter();
        singingClip = clip;
        parameterId = id;
        registered = isRegistered;
        parameterInfo = info;
        updateControllerDefinition();

        if (singingClip && !parameterId.isEmpty() && registered) {
            auto *parameterMap = singingClip->parameters();
            targetConnections.append(connect(parameterMap, &dspx::ParameterMap::itemInserted, this,
                [this](const QString &key, dspx::Parameter *item) {
                    if (key != parameterId)
                        return;
                    if (creatingParameter) {
                        parameter = item;
                        Q_EMIT q_ptr->parameterChanged();
                    } else {
                        abortActiveEdit();
                        bindParameter(item);
                    }
                }));
            targetConnections.append(connect(parameterMap, &dspx::ParameterMap::itemRemoved, this,
                [this](const QString &key, dspx::Parameter *item) {
                    if (key == parameterId && parameter == item) {
                        abortActiveEdit();
                        bindParameter(nullptr);
                    }
                }));
            targetConnections.append(connect(singingClip, &QObject::destroyed, this, [this] {
                setTarget(nullptr, {}, {}, false);
            }));
            bindParameter(parameterMap->item(parameterId));
        } else {
            clearViewModels();
        }

        if (targetChanged)
            Q_EMIT q_ptr->targetChanged();
        if (infoChanged)
            Q_EMIT q_ptr->parameterInfoChanged();
    }

    void ParameterViewModelBindingPrivate::abortActiveEdit() {
        if (currentOperation != NoOperation)
            abortOperation(currentOperation, currentOperationTransforms);
    }

    void ParameterViewModelBindingPrivate::updateControllerDefinition() {
        const double defaultValue = canonicalNormalizedValue(parameterInfo.invokeNormalize(parameterInfo.defaultValue));
        interactionController->setDefaultValue(defaultValue);
        interactionController->setFillBaseline(defaultValue);
        interactionController->setReferenceBaseline(defaultValue);
        interactionController->setFillMode(
            sflow::ParameterEditorInteractionController::FillMode(parameterInfo.fillMode));
        interactionController->setDefaultValueEnabled(parameterInfo.valueType == Core::ParameterInfo::Relative);
        interactionController->setReferenceVisible(parameterInfo.showDefaultValue);
        interactionController->setOriginalAndDefaultCurveDisplayMode(
            sflow::ParameterEditorInteractionController::CurveSolid);

        transformInteractionController->setDefaultValue(0.5);
        transformInteractionController->setFillBaseline(0.5);
        transformInteractionController->setReferenceBaseline(0.5);
        transformInteractionController->setFillMode(sflow::ParameterEditorInteractionController::NoFill);
        transformInteractionController->setDefaultValueEnabled(true);
        transformInteractionController->setReferenceVisible(false);
        transformInteractionController->setOriginalAndDefaultCurveDisplayMode(
            sflow::ParameterEditorInteractionController::CurveSolid);
    }

    void ParameterViewModelBindingPrivate::bindParameter(dspx::Parameter *newParameter) {
        if (parameter == newParameter && !parameterConnections.isEmpty())
            return;
        unbindParameter();
        parameter = newParameter;
        if (parameter)
            reloadFromDocument();
        else
            clearViewModels();
        Q_EMIT q_ptr->parameterChanged();
    }

    void ParameterViewModelBindingPrivate::unbindParameter() {
        disconnectAll(parameterConnections);
        parameter = nullptr;
        clearViewModels();
        resetDirtyState();
    }

    void ParameterViewModelBindingPrivate::reloadFromDocument() {
        disconnectAll(parameterConnections);
        clearViewModels();
        if (!parameter)
            return;
        bindFreeArray(parameter->original(), original);
        bindFreeArray(parameter->freeEdited(), freeEdited);
        bindFreeArray(parameter->freeTransform(), freeTransform, true);
        bindAnchorSequence(parameter->anchorEdited(), anchorEdited, true);
        bindAnchorSequence(parameter->anchorTransform(), anchorTransform, false);
        resetDirtyState();
    }

    void ParameterViewModelBindingPrivate::clearViewModels() {
        updatingView = true;
        original->splice(0, original->size(), {});
        freeEdited->splice(0, freeEdited->size(), {});
        freeTransform->splice(0, freeTransform->size(), {});
        for (auto *object : anchorEdited->items()) {
            auto *item = qobject_cast<sflow::ParameterAnchorViewModel *>(object);
            anchorEdited->removeItem(item);
            item->deleteLater();
        }
        for (auto *object : anchorTransform->items()) {
            auto *item = qobject_cast<sflow::ParameterAnchorViewModel *>(object);
            anchorTransform->removeItem(item);
            item->deleteLater();
        }
        anchorViewItems.clear();
        anchorDocumentItems.clear();
        transformAnchorViewItems.clear();
        transformAnchorDocumentItems.clear();
        pendingSelectedAnchors.clear();
        pendingCurrentAnchor = nullptr;
        updatingView = false;
    }

    void ParameterViewModelBindingPrivate::bindFreeArray(dspx::FreeValueDataArray *array,
                                                         sflow::FreeParameterViewModel *viewModel,
                                                         bool transform) {
        updatingView = true;
        viewModel->splice(0, viewModel->size(), normalizeValues(array->items(), transform));
        updatingView = false;
        parameterConnections.append(connect(array, &dspx::FreeValueDataArray::spliced, this,
            [this, viewModel, transform](int index, int removeCount, const QList<QVariant> &values) {
                if (updatingDocument)
                    return;
                updatingView = true;
                viewModel->splice(index, removeCount, normalizeValues(values, transform));
                updatingView = false;
            }));
        parameterConnections.append(connect(array, &dspx::FreeValueDataArray::rotated, this,
            [this, array, viewModel, transform] {
                if (updatingDocument)
                    return;
                updatingView = true;
                viewModel->splice(0, viewModel->size(), normalizeValues(array->items(), transform));
                updatingView = false;
            }));

    }

    void ParameterViewModelBindingPrivate::bindAnchorSequence(dspx::AnchorNodeSequence *sequence,
                                                               sflow::AnchorParameterViewModel *viewModel,
                                                               bool edited) {
        for (auto *item : sequence->asRange())
            bindAnchor(item, viewModel, edited);
        parameterConnections.append(connect(sequence, &dspx::AnchorNodeSequence::itemInserted, this,
            [this, viewModel, edited](dspx::AnchorNode *item) {
                if (!updatingDocument)
                    bindAnchor(item, viewModel, edited);
            }));
        parameterConnections.append(connect(sequence, &dspx::AnchorNodeSequence::itemRemoved, this,
            [this, viewModel, edited](dspx::AnchorNode *item) {
                if (!updatingDocument)
                    unbindAnchor(item, viewModel, edited);
            }));

    }

    void ParameterViewModelBindingPrivate::bindAnchor(dspx::AnchorNode *item,
                                                       sflow::AnchorParameterViewModel *viewModel,
                                                       bool edited,
                                                       sflow::ParameterAnchorViewModel *existingViewItem) {
        auto &viewMap = edited ? anchorViewItems : transformAnchorViewItems;
        auto &documentMap = edited ? anchorDocumentItems : transformAnchorDocumentItems;
        if (viewMap.contains(item))
            return;
        auto *viewItem = existingViewItem ? existingViewItem : new sflow::ParameterAnchorViewModel(viewModel);
        const auto info = conversionInfo(!edited);
        updatingView = true;
        viewItem->setPosition(item->x());
        viewItem->setValue(info.invokeNormalize(item->y()));
        viewItem->setInterpolationMode(toViewInterpolation(item->interpolationMode()));
        viewItem->setSelected(selectionModel->anchorNodeSelectionModel()->isItemSelected(item));
        if (!existingViewItem)
            viewModel->insertItem(viewItem);
        updatingView = false;
        viewMap.insert(item, viewItem);
        documentMap.insert(viewItem, item);

        parameterConnections.append(connect(item, &dspx::AnchorNode::xChanged, viewItem, [this, item, viewItem, viewModel] {
            if (updatingDocument || viewItem->position() == item->x())
                return;
            updatingView = true;
            viewModel->moveItem(viewItem, item->x());
            updatingView = false;
        }));
        parameterConnections.append(connect(item, &dspx::AnchorNode::yChanged, viewItem,
            [this, item, viewItem, edited] {
            if (updatingDocument)
                return;
            updatingView = true;
            viewItem->setValue(conversionInfo(!edited).invokeNormalize(item->y()));
            updatingView = false;
        }));
        parameterConnections.append(connect(item, &dspx::AnchorNode::interpolationModeChanged, viewItem,
            [this, item, viewItem] {
                if (updatingDocument)
                    return;
                updatingView = true;
                viewItem->setInterpolationMode(toViewInterpolation(item->interpolationMode()));
                updatingView = false;
            }));
    }

    void ParameterViewModelBindingPrivate::unbindAnchor(dspx::AnchorNode *item,
                                                         sflow::AnchorParameterViewModel *viewModel,
                                                         bool edited) {
        auto &viewMap = edited ? anchorViewItems : transformAnchorViewItems;
        auto &documentMap = edited ? anchorDocumentItems : transformAnchorDocumentItems;
        auto *viewItem = viewMap.take(item);
        if (!viewItem)
            return;
        documentMap.remove(viewItem);
        updatingView = true;
        viewModel->removeItem(viewItem);
        updatingView = false;
        disconnect(item, nullptr, viewItem, nullptr);
        viewItem->deleteLater();
    }

    Core::ParameterInfo ParameterViewModelBindingPrivate::conversionInfo(bool transform) const {
        return transform ? Core::transformParameterInfo() : parameterInfo;
    }

    QList<QVariant> ParameterViewModelBindingPrivate::normalizeValues(const QList<QVariant> &values,
                                                                      bool transform) const {
        const auto info = conversionInfo(transform);
        QList<QVariant> result;
        result.reserve(values.size());
        for (const auto &value : values)
            result.append(value.isValid() ? QVariant(info.invokeNormalize(value.toInt())) : QVariant());
        return result;
    }

    QList<QVariant> ParameterViewModelBindingPrivate::denormalizeValues(const QList<QVariant> &values,
                                                                        bool transform) const {
        QList<QVariant> result;
        result.reserve(values.size());
        for (const auto &value : values)
            result.append(value.isValid() ? QVariant(canonicalRawValue(value.toDouble(), transform)) : QVariant());
        return result;
    }

    int ParameterViewModelBindingPrivate::canonicalRawValue(double normalizedValue, bool transform) const {
        const auto info = conversionInfo(transform);
        const double bottom = info.invokeNormalize(info.bottomValue);
        const double top = info.invokeNormalize(info.topValue);
        if (!std::isfinite(normalizedValue)) {
            normalizedValue = info.invokeNormalize(info.defaultValue);
        } else if (std::isfinite(bottom) && std::isfinite(top)) {
            normalizedValue = std::clamp(normalizedValue, qMin(bottom, top), qMax(bottom, top));
        }
        return boundedRawValue(info, info.invokeDenormalize(normalizedValue));
    }

    double ParameterViewModelBindingPrivate::canonicalNormalizedValue(double normalizedValue,
                                                                       bool transform) const {
        const auto info = conversionInfo(transform);
        return info.invokeNormalize(canonicalRawValue(normalizedValue, transform));
    }

    sflow::ParameterAnchorViewModel::InterpolationMode
    ParameterViewModelBindingPrivate::toViewInterpolation(int mode) {
        switch (dspx::AnchorNode::InterpolationMode(mode)) {
            case dspx::AnchorNode::Hermite:
                return sflow::ParameterAnchorViewModel::Hermite;
            case dspx::AnchorNode::Linear:
                return sflow::ParameterAnchorViewModel::Linear;
            case dspx::AnchorNode::None:
            default:
                return sflow::ParameterAnchorViewModel::None;
        }
    }

    int ParameterViewModelBindingPrivate::toDocumentInterpolation(
        sflow::ParameterAnchorViewModel::InterpolationMode mode) {
        switch (mode) {
            case sflow::ParameterAnchorViewModel::Hermite:
                return dspx::AnchorNode::Hermite;
            case sflow::ParameterAnchorViewModel::Linear:
                return dspx::AnchorNode::Linear;
            case sflow::ParameterAnchorViewModel::None:
            default:
                return dspx::AnchorNode::None;
        }
    }

    void ParameterViewModelBindingPrivate::startOperation(Operation operation, bool transform) {
        if (!editable || !singingClip || parameterId.isEmpty() || !registered ||
            currentOperation != NoOperation || !stateMachine->configuration().contains(idleState)) {
            return;
        }
        transactionId = document->transactionController()->beginTransaction();
        if (transactionId == Core::TransactionController::TransactionId::Invalid) {
            QMetaObject::invokeMethod(q_ptr, [this] {
                if (currentOperation != NoOperation)
                    return;
                if (parameter)
                    reloadFromDocument();
                else
                    clearViewModels();
                resetDirtyState();
            }, Qt::QueuedConnection);
            return;
        }
        currentOperation = operation;
        currentOperationTransforms = transform;
        emitWillStart(operation);
    }

    void ParameterViewModelBindingPrivate::commitOperation(Operation operation, bool transform) {
        if (currentOperation == operation && currentOperationTransforms == transform)
            emitWillCommit(operation);
    }

    void ParameterViewModelBindingPrivate::abortOperation(Operation operation, bool transform) {
        if (currentOperation == operation && currentOperationTransforms == transform)
            emitWillAbort(operation);
    }

    void ParameterViewModelBindingPrivate::enterPending(Operation operation) {
        if (transactionId == Core::TransactionController::TransactionId::Invalid) {
            currentOperation = NoOperation;
            emitNotStarted(operation);
        } else {
            emitStarted(operation);
        }
    }

    void ParameterViewModelBindingPrivate::enterCommitting(Operation) {
        if (transactionId == Core::TransactionController::TransactionId::Invalid) {
            if (parameter)
                reloadFromDocument();
            else
                clearViewModels();
            resetDirtyState();
            return;
        }
        const bool changed = commitChanges();
        const bool finalized = changed
            ? document->transactionController()->commitTransaction(transactionId,
                  currentOperationTransforms ? tr("Editing parameter transform")
                                             : tr("Editing parameter"))
            : document->transactionController()->abortTransaction(transactionId);
        if (!finalized) {
            qCCritical(lcParameterEditorContext) << parameterId
                << "failed to finalize parameter transaction";
            document->transactionController()->abortTransaction(transactionId);
        }
        transactionId = {};
        const QPointer<dspx::Parameter> committedParameter = parameter;
        QMetaObject::invokeMethod(q_ptr, [this, committedParameter] {
            if (parameter != committedParameter)
                return;
            if (parameter)
                reloadFromDocument();
            else
                clearViewModels();
        }, Qt::QueuedConnection);
    }

    void ParameterViewModelBindingPrivate::enterAborting(Operation) {
        if (transactionId != Core::TransactionController::TransactionId::Invalid &&
            !document->transactionController()->abortTransaction(transactionId)) {
            qCCritical(lcParameterEditorContext) << parameterId
                << "failed to abort parameter transaction";
        }
        transactionId = {};
        if (parameter)
            reloadFromDocument();
        else
            clearViewModels();
        resetDirtyState();
    }

    bool ParameterViewModelBindingPrivate::commitChanges() {
        if (!hasChanges())
            return false;
        const bool creating = !parameter;
        if (creating && !createParameter())
            return false;
        updatingDocument = true;
        const bool freeChanged = commitFreeChanges();
        const bool anchorChanged = commitAnchorChanges();
        updatingDocument = false;
        const bool changed = freeChanged || anchorChanged;
        resetDirtyState();
        return changed;
    }

    dspx::Parameter *ParameterViewModelBindingPrivate::createParameter() {
        if (!singingClip || parameterId.isEmpty())
            return nullptr;
        auto *newParameter = model->createParameter();
        creatingParameter = true;
        if (!singingClip->parameters()->insertItem(parameterId, newParameter)) {
            creatingParameter = false;
            model->destroyItem(newParameter);
            return nullptr;
        }
        creatingParameter = false;
        parameter = newParameter;
        return parameter;
    }

    bool ParameterViewModelBindingPrivate::commitFreeChanges() {
        if (!parameter || freeDirtyLast <= freeDirtyFirst)
            return false;
        auto *viewValues = currentOperationTransforms ? freeTransform : freeEdited;
        auto *documentValues = currentOperationTransforms ? parameter->freeTransform()
                                                          : parameter->freeEdited();
        const int dirtyFirst = qMax(0, qMin(freeDirtyFirst, viewValues->size()));
        const int last = qMax(dirtyFirst, qMin(freeDirtyLast, viewValues->size()));
        const int first = qMin(dirtyFirst, documentValues->size());
        const int count = last - first;
        if (count <= 0)
            return false;
        const auto normalizedValues = viewValues->slice(first, count);
        const auto rawValues = denormalizeValues(normalizedValues, currentOperationTransforms);
        const int removeCount = qMin(count, qMax(0, documentValues->size() - first));
        return documentValues->splice(first, removeCount, rawValues);
    }

    bool ParameterViewModelBindingPrivate::commitAnchorChanges() {
        if (!parameter || (dirtyAnchors.isEmpty() && insertedAnchors.isEmpty() && removedAnchors.isEmpty()))
            return false;
        auto *viewModel = currentOperationTransforms ? anchorTransform : anchorEdited;
        auto *sequence = currentOperationTransforms ? parameter->anchorTransform()
                                                    : parameter->anchorEdited();
        auto &documentItems = currentOperationTransforms ? transformAnchorDocumentItems
                                                         : anchorDocumentItems;
        auto &viewItems = currentOperationTransforms ? transformAnchorViewItems : anchorViewItems;
        QSet<dspx::AnchorNode *> movingDocumentItems;
        struct Entry {
            sflow::ParameterAnchorViewModel *viewItem{};
            dspx::AnchorNode *documentItem{};
            int position{};
            int value{};
            dspx::AnchorNode::InterpolationMode interpolation{dspx::AnchorNode::None};
            bool selected{};
            bool current{};
        };
        QList<Entry> entries;
        QSet<sflow::ParameterAnchorViewModel *> changedViews = dirtyAnchors;
        changedViews.unite(insertedAnchors);
        entries.reserve(changedViews.size());
        for (auto *viewItem : std::as_const(changedViews)) {
            if (!viewItem || viewItem->anchorParameterViewModel() != viewModel)
                continue;
            auto *documentItem = documentItems.value(viewItem);
            if (documentItem)
                movingDocumentItems.insert(documentItem);
            entries.append({
                viewItem,
                documentItem,
                qMax(0, viewItem->position()),
                canonicalRawValue(viewItem->value(), currentOperationTransforms),
                dspx::AnchorNode::InterpolationMode(toDocumentInterpolation(viewItem->interpolationMode())),
                viewItem->isSelected() || pendingSelectedAnchors.contains(viewItem),
                pendingCurrentAnchor == viewItem ||
                    selectionModel->anchorNodeSelectionModel()->currentItem() == documentItem,
            });
        }

        for (const auto &entry : std::as_const(entries)) {
            for (auto *overlap : sequence->slice(entry.position, 1)) {
                if (movingDocumentItems.contains(overlap))
                    continue;
                sequence->removeItem(overlap);
                if (auto *overlapView = viewItems.take(overlap)) {
                    documentItems.remove(overlapView);
                    updatingView = true;
                    if (overlapView->anchorParameterViewModel() == viewModel)
                        viewModel->removeItem(overlapView);
                    updatingView = false;
                    overlapView->deleteLater();
                }
                removedAnchors.remove(overlap);
                model->destroyItem(overlap);
            }
        }
        for (auto *item : std::as_const(movingDocumentItems))
            sequence->removeItem(item);
        for (auto *item : std::as_const(removedAnchors)) {
            if (sequence->contains(item))
                sequence->removeItem(item);
            model->destroyItem(item);
        }

        for (auto &entry : entries) {
            if (!entry.documentItem)
                entry.documentItem = model->createAnchorNode();
            entry.documentItem->setX(entry.position);
            entry.documentItem->setY(entry.value);
            entry.documentItem->setInterpolationMode(entry.interpolation);
            if (!sequence->insertItem(entry.documentItem)) {
                if (!movingDocumentItems.contains(entry.documentItem))
                    model->destroyItem(entry.documentItem);
                continue;
            }
            documentItems.insert(entry.viewItem, entry.documentItem);
            viewItems.insert(entry.documentItem, entry.viewItem);
            dspx::SelectionModel::SelectionCommand selectionCommand;
            if (entry.selected)
                selectionCommand |= dspx::SelectionModel::Select;
            if (entry.current)
                selectionCommand |= dspx::SelectionModel::SetCurrentItem;
            if (selectionCommand) {
                selectionModel->select(entry.documentItem,
                    selectionCommand, dspx::SelectionModel::ST_AnchorNode, sequence);
            }
            updatingView = true;
            entry.viewItem->setValue(conversionInfo(currentOperationTransforms).invokeNormalize(entry.value));
            updatingView = false;
        }
        return !entries.isEmpty() || !removedAnchors.isEmpty();
    }

    bool ParameterViewModelBindingPrivate::hasChanges() const {
        return freeDirtyLast > freeDirtyFirst || !dirtyAnchors.isEmpty() ||
               !insertedAnchors.isEmpty() || !removedAnchors.isEmpty();
    }

    void ParameterViewModelBindingPrivate::resetDirtyState() {
        freeDirtyFirst = std::numeric_limits<int>::max();
        freeDirtyLast = -1;
        dirtyAnchors.clear();
        insertedAnchors.clear();
        for (auto *item : std::as_const(removedAnchorViews))
            item->deleteLater();
        removedAnchorViews.clear();
        removedAnchors.clear();
        pendingSelectedAnchors.clear();
        pendingCurrentAnchor = nullptr;
    }

#define EMIT_OPERATION_SIGNAL(FunctionName, SignalSuffix) \
    void ParameterViewModelBindingPrivate::FunctionName(Operation operation) { \
        switch (operation) { \
            case FreeEditing: Q_EMIT free##SignalSuffix(); break; \
            case AnchorInsertion: Q_EMIT insertion##SignalSuffix(); break; \
            case AnchorMoving: Q_EMIT moving##SignalSuffix(); break; \
            case AnchorDeletion: Q_EMIT deletion##SignalSuffix(); break; \
            case AnchorConversion: Q_EMIT conversion##SignalSuffix(); break; \
            case NoOperation: break; \
        } \
    }
    EMIT_OPERATION_SIGNAL(emitWillStart, WillStart)
    EMIT_OPERATION_SIGNAL(emitStarted, Started)
    EMIT_OPERATION_SIGNAL(emitNotStarted, NotStarted)
    EMIT_OPERATION_SIGNAL(emitWillCommit, WillCommit)
    EMIT_OPERATION_SIGNAL(emitWillAbort, WillAbort)
#undef EMIT_OPERATION_SIGNAL

    ParameterViewModelBinding::ParameterViewModelBinding(QObject *parent, bool editable)
        : QObject(parent), d_ptr(new ParameterViewModelBindingPrivate(this, editable)) {
    }

    ParameterViewModelBinding::~ParameterViewModelBinding() = default;

    QString ParameterViewModelBinding::parameterId() const { Q_D(const ParameterViewModelBinding); return d->parameterId; }
    Core::ParameterInfo ParameterViewModelBinding::parameterInfo() const { Q_D(const ParameterViewModelBinding); return d->parameterInfo; }
    Core::ParameterInfo ParameterViewModelBinding::transformParameterInfo() const { return Core::transformParameterInfo(); }
    bool ParameterViewModelBinding::isRegistered() const { Q_D(const ParameterViewModelBinding); return d->registered; }
    bool ParameterViewModelBinding::isAvailable() const { Q_D(const ParameterViewModelBinding); return d->singingClip && !d->parameterId.isEmpty() && d->registered; }
    bool ParameterViewModelBinding::parameterExists() const { Q_D(const ParameterViewModelBinding); return d->parameter; }
    sflow::FreeParameterViewModel *ParameterViewModelBinding::original() const { Q_D(const ParameterViewModelBinding); return d->original; }
    sflow::FreeParameterViewModel *ParameterViewModelBinding::freeEdited() const { Q_D(const ParameterViewModelBinding); return d->freeEdited; }
    sflow::FreeParameterViewModel *ParameterViewModelBinding::freeTransform() const { Q_D(const ParameterViewModelBinding); return d->freeTransform; }
    sflow::AnchorParameterViewModel *ParameterViewModelBinding::anchorEdited() const { Q_D(const ParameterViewModelBinding); return d->anchorEdited; }
    sflow::AnchorParameterViewModel *ParameterViewModelBinding::anchorTransform() const { Q_D(const ParameterViewModelBinding); return d->anchorTransform; }
    sflow::ParameterRangeSelectionViewModel *ParameterViewModelBinding::freeSelection() const { Q_D(const ParameterViewModelBinding); return d->freeSelection; }
    sflow::ParameterRangeSelectionViewModel *ParameterViewModelBinding::transformFreeSelection() const { Q_D(const ParameterViewModelBinding); return d->transformFreeSelection; }
    sflow::SelectionController *ParameterViewModelBinding::anchorSelectionController() const { Q_D(const ParameterViewModelBinding); return d->anchorSelectionController; }
    sflow::SelectionController *ParameterViewModelBinding::transformAnchorSelectionController() const { Q_D(const ParameterViewModelBinding); return d->transformAnchorSelectionController; }
    sflow::ParameterEditorInteractionController *ParameterViewModelBinding::interactionController() const { Q_D(const ParameterViewModelBinding); return d->interactionController; }
    sflow::ParameterEditorInteractionController *ParameterViewModelBinding::transformInteractionController() const { Q_D(const ParameterViewModelBinding); return d->transformInteractionController; }

    void ParameterViewModelBinding::focusFreeLayer() {
        Q_D(ParameterViewModelBinding);
        if (!d->editable || !d->singingClip || d->parameterId.isEmpty() || !d->registered)
            return;
        d->freeSelectionModel->setContext(d->singingClip, d->parameterId,
            d->parameterInfo.displayName.isEmpty() ? d->parameterId : d->parameterInfo.displayName,
            Core::FreeParameterSelectionModel::EditedLayer);
    }

    void ParameterViewModelBinding::focusAnchorLayer() {
        Q_D(ParameterViewModelBinding);
        if (!d->editable || !d->singingClip || d->parameterId.isEmpty() || !d->registered)
            return;
        if (d->freeSelectionModel->singingClip() == d->singingClip &&
            d->freeSelectionModel->parameterId() == d->parameterId) {
            d->freeSelectionModel->clearContext();
        }
        if (d->parameter &&
            d->selectionModel->selectionType() == dspx::SelectionModel::ST_AnchorNode &&
            d->selectionModel->anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems() ==
                d->parameter->anchorEdited()) {
            return;
        }
        d->selectionModel->select(nullptr, dspx::SelectionModel::ClearPreviousSelection,
                                  d->parameter ? dspx::SelectionModel::ST_AnchorNode
                                               : dspx::SelectionModel::ST_None,
                                  d->parameter ? d->parameter->anchorEdited() : nullptr);
    }

    void ParameterViewModelBinding::focusTransformFreeLayer() {
        Q_D(ParameterViewModelBinding);
        if (!d->editable || !d->singingClip || d->parameterId.isEmpty() || !d->registered)
            return;
        const auto name = d->parameterInfo.displayName.isEmpty() ? d->parameterId
                                                                 : d->parameterInfo.displayName;
        d->freeSelectionModel->setContext(d->singingClip, d->parameterId,
            tr("%1 Transform").arg(name), Core::FreeParameterSelectionModel::TransformLayer);
    }

    void ParameterViewModelBinding::focusTransformAnchorLayer() {
        Q_D(ParameterViewModelBinding);
        if (!d->editable || !d->singingClip || d->parameterId.isEmpty() || !d->registered)
            return;
        if (d->freeSelectionModel->singingClip() == d->singingClip &&
            d->freeSelectionModel->parameterId() == d->parameterId) {
            d->freeSelectionModel->clearContext();
        }
        if (d->parameter &&
            d->selectionModel->selectionType() == dspx::SelectionModel::ST_AnchorNode &&
            d->selectionModel->anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems() ==
                d->parameter->anchorTransform()) {
            return;
        }
        d->selectionModel->select(nullptr, dspx::SelectionModel::ClearPreviousSelection,
                                  d->parameter ? dspx::SelectionModel::ST_AnchorNode
                                               : dspx::SelectionModel::ST_None,
                                  d->parameter ? d->parameter->anchorTransform() : nullptr);
    }

    void ParameterEditorContextPrivate::initialize() {
        Q_Q(ParameterEditorContext);
        document = projectContext->windowHandle()->projectDocumentContext()->document();
        registry = Core::CoreInterface::singerRegistry();
        parameterModel = new ParameterDefinitionListModel(q);
        pitchBinding = new ParameterViewModelBinding(q, true);
        editingBinding = new ParameterViewModelBinding(q, true);
        referenceBinding = new ParameterViewModelBinding(q, false);
        pitchBinding->d_func()->initialize(document);
        editingBinding->d_func()->initialize(document);
        referenceBinding->d_func()->initialize(document);

        auto *noteSelection = document->selectionModel()->noteSelectionModel();
        QObject::connect(noteSelection, &dspx::NoteSelectionModel::noteSequenceWithSelectedItemsChanged,
                         q, [this, noteSelection] {
            if (auto *sequence = noteSelection->noteSequenceWithSelectedItems())
                setSingingClip(sequence->singingClip());
        });
        auto refreshArchitecture = [this](const QString &architectureId_) {
            if (architectureId_ != architectureId)
                return;
            rebuildParameterModel();
            refreshBindings();
        };
        QObject::connect(registry, &Core::SingerRegistry::architectureRegistered, q, refreshArchitecture);
        QObject::connect(registry, &Core::SingerRegistry::architectureUpdated, q, refreshArchitecture);
        QObject::connect(registry, &Core::SingerRegistry::architectureRemoved, q, refreshArchitecture);

        rebuildParameterModel();
        refreshBindings();
    }

    void ParameterEditorContextPrivate::setSingingClip(dspx::SingingClip *clip) {
        Q_Q(ParameterEditorContext);
        if (singingClip == clip)
            return;
        pitchBinding->d_func()->abortActiveEdit();
        editingBinding->d_func()->abortActiveEdit();
        disconnectAll(clipConnections);
        disconnectAll(sourceConnections);
        singingClip = clip;
        reconnectClip();
        reconnectSources();
        clearParameterSelection();
        rebuildParameterModel();
        refreshBindings();
        Q_EMIT q->singingClipChanged();
    }

    void ParameterEditorContextPrivate::reconnectClip() {
        if (!singingClip)
            return;
        clipConnections.append(QObject::connect(singingClip, &dspx::SingingClip::sourcesChanged,
            q_ptr, [this] {
                reconnectSources();
                rebuildParameterModel();
                refreshBindings();
            }));
        auto *parameters = singingClip->parameters();
        clipConnections.append(QObject::connect(parameters, &dspx::ParameterMap::itemInserted,
            q_ptr, [this](const QString &, dspx::Parameter *) {
                rebuildParameterModel();
                refreshBindings();
            }));
        clipConnections.append(QObject::connect(parameters, &dspx::ParameterMap::itemRemoved,
            q_ptr, [this](const QString &, dspx::Parameter *) {
                rebuildParameterModel();
                refreshBindings();
            }));
        clipConnections.append(QObject::connect(singingClip, &QObject::destroyed, q_ptr, [this] {
            setSingingClip(nullptr);
        }));
    }

    void ParameterEditorContextPrivate::reconnectSources() {
        disconnectAll(sourceConnections);
        sources = singingClip ? singingClip->sources() : nullptr;
        architectureId = sources ? sources->category() : QString{};
        if (!sources)
            return;
        sourceConnections.append(QObject::connect(sources, &dspx::Sources::categoryChanged,
            q_ptr, [this](const QString &category) {
                architectureId = category;
                rebuildParameterModel();
                refreshBindings();
            }));
        sourceConnections.append(QObject::connect(sources, &QObject::destroyed, q_ptr, [this] {
            sources = nullptr;
            architectureId.clear();
            rebuildParameterModel();
            refreshBindings();
        }));
    }

    void ParameterEditorContextPrivate::rebuildParameterModel() {
        const QString oldEditingParameterId = editingParameterId;
        const QString oldReferenceParameterId = referenceParameterId;
        const QString oldEditingDisplayName = displayName(editingParameterId, editingDisplayName);
        const QString oldReferenceDisplayName = displayName(referenceParameterId, referenceDisplayName);
        QList<ParameterDefinitionListModel::Entry> entries;
        entries.append({{}, ParameterEditorContext::tr("None"), {}, true, true});
        QSet<QString> included;
        included.insert(QStringLiteral("pitch"));
        if (registry->containsArchitecture(architectureId)) {
            const auto parameters = registry->architectureInfo(architectureId).parameters();
            for (auto it = parameters.cbegin(); it != parameters.cend(); ++it) {
                if (it.key() == QStringLiteral("pitch"))
                    continue;
                entries.append({it.key(), it.value().displayName.isEmpty() ? it.key() : it.value().displayName,
                                it.value(), true, false});
                included.insert(it.key());
            }
        }
        if (singingClip) {
            for (const auto &key : singingClip->parameters()->keys()) {
                if (included.contains(key))
                    continue;
                entries.append({key, key, {}, false, false});
                included.insert(key);
            }
        }
        parameterModel->setEntries(std::move(entries));

        const auto *editingDefinition = entry(editingParameterId);
        if ((!editingDefinition || !editingDefinition->registered || editingParameterId.isEmpty()) &&
            singingClip) {
            if (const auto *firstAvailable = parameterModel->firstAvailable()) {
                editingParameterId = firstAvailable->parameterId;
                editingDefinition = firstAvailable;
            }
        }

        const auto *referenceDefinition = entry(referenceParameterId);
        if (!referenceParameterId.isEmpty() &&
            (!referenceDefinition || !referenceDefinition->registered)) {
            referenceParameterId.clear();
            referenceDefinition = entry(referenceParameterId);
        }

        if (editingDefinition && editingDefinition->registered) {
            const auto *definition = editingDefinition;
            editingDisplayName = definition->displayName;
            editingLastInfo = definition->parameterInfo;
        }
        if (referenceDefinition && referenceDefinition->registered && !referenceDefinition->none) {
            const auto *definition = referenceDefinition;
            referenceDisplayName = definition->displayName;
            referenceLastInfo = definition->parameterInfo;
        }

        const bool editingParameterChanged = oldEditingParameterId != editingParameterId;
        const bool referenceParameterChanged = oldReferenceParameterId != referenceParameterId;
        if (editingParameterChanged) {
            clearParameterSelection();
            restoreEditingSelectionAfterRefresh = true;
        }
        if (editingParameterChanged ||
            oldEditingDisplayName != displayName(editingParameterId, editingDisplayName)) {
            Q_EMIT q_ptr->editingParameterIdChanged();
        }
        if (referenceParameterChanged ||
            oldReferenceDisplayName != displayName(referenceParameterId, referenceDisplayName)) {
            Q_EMIT q_ptr->referenceParameterIdChanged();
        }
        if (editingParameterChanged || referenceParameterChanged)
            Q_EMIT q_ptr->referenceDataBindingChanged();
    }

    void ParameterEditorContextPrivate::refreshBindings() {
        const auto update = [this](ParameterViewModelBinding *binding, const QString &id,
                                   const Core::ParameterInfo &lastInfo) {
            if (id.isEmpty()) {
                binding->d_func()->setTarget(singingClip, {}, {}, false);
                return;
            }
            if (const auto *definition = entry(id); definition && definition->registered) {
                binding->d_func()->setTarget(singingClip, id, definition->parameterInfo, true);
            } else {
                binding->d_func()->setTarget(singingClip, id, lastInfo, false);
            }
        };
        pitchBinding->d_func()->setTarget(singingClip, QStringLiteral("pitch"),
                                          Core::pitchParameterInfo(), true);
        update(editingBinding, editingParameterId, editingLastInfo);
        if (!referenceParameterId.isEmpty() && referenceParameterId == editingParameterId)
            referenceBinding->d_func()->setTarget(nullptr, {}, {}, false);
        else
            update(referenceBinding, referenceParameterId, referenceLastInfo);

        if (restoreEditingSelectionAfterRefresh) {
            restoreEditingSelectionAfterRefresh = false;
            if (editingBinding->parameterExists()) {
                auto *parameter = singingClip->parameters()->item(editingParameterId);
                document->selectionModel()->select(nullptr,
                    dspx::SelectionModel::ClearPreviousSelection,
                    dspx::SelectionModel::ST_AnchorNode,
                    transformEditing ? parameter->anchorTransform() : parameter->anchorEdited());
            }
        }
    }

    void ParameterEditorContextPrivate::abortEditingOperation() {
        editingBinding->d_func()->abortActiveEdit();
    }

    void ParameterEditorContextPrivate::clearParameterSelection() {
        auto *selection = document->selectionModel();
        selection->select(nullptr, dspx::SelectionModel::ClearPreviousSelection,
                          dspx::SelectionModel::ST_None);
        document->freeParameterSelectionModel()->clearContext();
        pitchBinding->freeSelection()->clear();
        pitchBinding->transformFreeSelection()->clear();
        editingBinding->freeSelection()->clear();
        editingBinding->transformFreeSelection()->clear();
    }

    const ParameterDefinitionListModel::Entry *ParameterEditorContextPrivate::entry(
        const QString &parameterId) const {
        return parameterModel->find(parameterId);
    }

    QString ParameterEditorContextPrivate::displayName(const QString &parameterId,
                                                        const QString &remembered) const {
        if (parameterId.isEmpty())
            return ParameterEditorContext::tr("None");
        if (const auto *definition = entry(parameterId))
            return definition->displayName;
        return remembered.isEmpty() ? parameterId : remembered;
    }

    ParameterEditorContext::ParameterEditorContext(ProjectViewModelContext *projectContext)
        : QObject(projectContext), d_ptr(new ParameterEditorContextPrivate) {
        Q_D(ParameterEditorContext);
        d->q_ptr = this;
        d->projectContext = projectContext;
        d->initialize();
    }

    ParameterEditorContext::~ParameterEditorContext() = default;

    dspx::SingingClip *ParameterEditorContext::singingClip() const { Q_D(const ParameterEditorContext); return d->singingClip; }
    QAbstractItemModel *ParameterEditorContext::parameterModel() const { Q_D(const ParameterEditorContext); return d->parameterModel; }
    QString ParameterEditorContext::editingParameterId() const { Q_D(const ParameterEditorContext); return d->editingParameterId; }

    void ParameterEditorContext::setEditingParameterId(const QString &parameterId) {
        Q_D(ParameterEditorContext);
        if (d->editingParameterId == parameterId)
            return;
        if (const auto *definition = d->entry(parameterId)) {
            d->editingDisplayName = definition->displayName;
            if (definition->registered)
                d->editingLastInfo = definition->parameterInfo;
        } else if (!parameterId.isEmpty()) {
            d->editingDisplayName = parameterId;
        }
        d->editingParameterId = parameterId;
        d->clearParameterSelection();
        d->refreshBindings();
        if (d->editingBinding->parameterExists()) {
            auto *parameter = d->singingClip->parameters()->item(parameterId);
            d->document->selectionModel()->select(nullptr,
                dspx::SelectionModel::ClearPreviousSelection,
                dspx::SelectionModel::ST_AnchorNode,
                d->transformEditing ? parameter->anchorTransform() : parameter->anchorEdited());
        }
        Q_EMIT editingParameterIdChanged();
        Q_EMIT referenceDataBindingChanged();
    }

    QString ParameterEditorContext::referenceParameterId() const { Q_D(const ParameterEditorContext); return d->referenceParameterId; }

    void ParameterEditorContext::setReferenceParameterId(const QString &parameterId) {
        Q_D(ParameterEditorContext);
        if (d->referenceParameterId == parameterId)
            return;
        if (const auto *definition = d->entry(parameterId)) {
            d->referenceDisplayName = definition->displayName;
            if (definition->registered)
                d->referenceLastInfo = definition->parameterInfo;
        } else if (!parameterId.isEmpty()) {
            d->referenceDisplayName = parameterId;
        }
        d->referenceParameterId = parameterId;
        d->refreshBindings();
        Q_EMIT referenceParameterIdChanged();
        Q_EMIT referenceDataBindingChanged();
    }

    QString ParameterEditorContext::editingParameterDisplayName() const { Q_D(const ParameterEditorContext); return d->displayName(d->editingParameterId, d->editingDisplayName); }
    QString ParameterEditorContext::referenceParameterDisplayName() const { Q_D(const ParameterEditorContext); return d->displayName(d->referenceParameterId, d->referenceDisplayName); }
    bool ParameterEditorContext::transformEditing() const { Q_D(const ParameterEditorContext); return d->transformEditing; }

    void ParameterEditorContext::setTransformEditing(bool transformEditing) {
        Q_D(ParameterEditorContext);
        if (d->transformEditing == transformEditing)
            return;
        d->abortEditingOperation();
        d->transformEditing = transformEditing;
        d->clearParameterSelection();
        if (d->editingBinding->parameterExists()) {
            auto *parameter = d->singingClip->parameters()->item(d->editingParameterId);
            d->document->selectionModel()->select(nullptr,
                dspx::SelectionModel::ClearPreviousSelection,
                dspx::SelectionModel::ST_AnchorNode,
                transformEditing ? parameter->anchorTransform() : parameter->anchorEdited());
        }
        Q_EMIT transformEditingChanged();
    }

    ParameterViewModelBinding *ParameterEditorContext::pitchBinding() const { Q_D(const ParameterEditorContext); return d->pitchBinding; }
    ParameterViewModelBinding *ParameterEditorContext::editingBinding() const { Q_D(const ParameterEditorContext); return d->editingBinding; }
    ParameterViewModelBinding *ParameterEditorContext::referenceBinding() const { Q_D(const ParameterEditorContext); return d->referenceBinding; }
    ParameterViewModelBinding *ParameterEditorContext::referenceDataBinding() const { Q_D(const ParameterEditorContext); return d->editingParameterId == d->referenceParameterId ? d->editingBinding : d->referenceBinding; }

    void ParameterEditorContext::setSingingClip(dspx::SingingClip *singingClip) {
        Q_D(ParameterEditorContext);
        d->setSingingClip(singingClip);
    }

    void ParameterEditorContext::swapParameters() {
        Q_D(ParameterEditorContext);
        const auto editingId = d->editingParameterId;
        const auto editingName = d->editingDisplayName;
        const auto editingInfo = d->editingLastInfo;
        d->editingParameterId = d->referenceParameterId;
        d->editingDisplayName = d->referenceDisplayName;
        d->editingLastInfo = d->referenceLastInfo;
        d->referenceParameterId = editingId;
        d->referenceDisplayName = editingName;
        d->referenceLastInfo = editingInfo;
        d->clearParameterSelection();
        d->refreshBindings();
        if (d->editingBinding->parameterExists()) {
            auto *parameter = d->singingClip->parameters()->item(d->editingParameterId);
            d->document->selectionModel()->select(nullptr,
                dspx::SelectionModel::ClearPreviousSelection,
                dspx::SelectionModel::ST_AnchorNode,
                d->transformEditing ? parameter->anchorTransform() : parameter->anchorEdited());
        }
        Q_EMIT editingParameterIdChanged();
        Q_EMIT referenceParameterIdChanged();
        Q_EMIT referenceDataBindingChanged();
    }
}

#include "moc_ParameterEditorContext.cpp"
