#include <visualeditor/DynamicMixingEditorContext.h>
#include <visualeditor/private/DynamicMixingEditorContext_p.h>

#include <algorithm>
#include <limits>
#include <utility>

#include <QState>
#include <QStateMachine>

#include <ScopicFlowCore/DynamicMixingAnchorViewModel.h>
#include <ScopicFlowCore/DynamicMixingEditorInteractionController.h>
#include <ScopicFlowCore/DynamicMixingViewModel.h>

#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelSelectionModel/DynamicMixingAnchorSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    DynamicMixingSelectionController::DynamicMixingSelectionController(
        DynamicMixingEditorContext *context)
        : SelectionController(context), context(context) {
        auto *d = context->d_func();
        connect(d->dynamicSelectionModel,
                &dspx::DynamicMixingAnchorSelectionModel::currentItemChanged,
                this, &SelectionController::currentItemChanged);
        connect(d->dynamicSelectionModel,
                &dspx::DynamicMixingAnchorSelectionModel::dynamicMixingAnchorSequenceWithSelectedItemsChanged,
                this, &SelectionController::editScopeFocusedChanged);
        connect(d->selectionModel, &dspx::SelectionModel::selectionTypeChanged,
                this, &SelectionController::editScopeFocusedChanged);
    }

    QObjectList DynamicMixingSelectionController::getSelectedItems() const {
        const auto *d = context->d_func();
        QObjectList result;
        for (auto *item : d->dynamicSelectionModel->selectedItems()) {
            if (auto *viewItem = d->viewItemMap.value(item))
                result.append(viewItem);
        }
        return result;
    }

    QObjectList DynamicMixingSelectionController::getItemsBetween(QObject *startItem,
                                                                    QObject *endItem) const {
        const auto *d = context->d_func();
        if (!d->viewModel || !d->viewModel->count())
            return {};
        auto *start = qobject_cast<sflow::DynamicMixingAnchorViewModel *>(startItem);
        auto *end = qobject_cast<sflow::DynamicMixingAnchorViewModel *>(endItem);
        const int first = start ? start->position()
                                : d->viewModel->itemAtOrAfterPosition(0)->position();
        auto *lastItem = end ? end : d->viewModel->itemAtOrBeforePosition(
                                      std::numeric_limits<int>::max());
        if (!lastItem)
            return {};
        const int last = lastItem->position();
        return d->viewModel->slice(std::min(first, last),
                                   std::max(first, last) - std::min(first, last) + 1);
    }

    void DynamicMixingSelectionController::select(QObject *item, SelectionCommand command) {
        auto *d = context->d_func();
        dspx::SelectionModel::SelectionCommand documentCommand{};
        if (command.testFlag(Select))
            documentCommand |= dspx::SelectionModel::Select;
        if (command.testFlag(Deselect))
            documentCommand |= dspx::SelectionModel::Deselect;
        if (command.testFlag(ClearPreviousSelection))
            documentCommand |= dspx::SelectionModel::ClearPreviousSelection;
        if (command.testFlag(SetCurrentItem))
            documentCommand |= dspx::SelectionModel::SetCurrentItem;
        auto *viewItem = qobject_cast<sflow::DynamicMixingAnchorViewModel *>(item);
        auto *documentItem = viewItem ? d->documentItemMap.value(viewItem) : nullptr;
        d->selectionModel->select(documentItem, documentCommand,
                                  dspx::SelectionModel::ST_DynamicMixingAnchor,
                                  d->sequence);
    }

    QObject *DynamicMixingSelectionController::currentItem() const {
        const auto *d = context->d_func();
        return d->viewItemMap.value(d->dynamicSelectionModel->currentItem());
    }

    bool DynamicMixingSelectionController::editScopeFocused() const {
        const auto *d = context->d_func();
        return d->selectionModel->selectionType()
                   == dspx::SelectionModel::ST_DynamicMixingAnchor
               && d->dynamicSelectionModel
                      ->dynamicMixingAnchorSequenceWithSelectedItems() == d->sequence;
    }

    void DynamicMixingEditorContextPrivate::initialize() {
        Q_Q(DynamicMixingEditorContext);
        document = projectContext->windowHandle()->projectDocumentContext()->document();
        selectionModel = document->selectionModel();
        dynamicSelectionModel = selectionModel->dynamicMixingAnchorSelectionModel();
        viewModel = new sflow::DynamicMixingViewModel(q);
        interactionController = new sflow::DynamicMixingEditorInteractionController(q);
        viewSelectionController = new DynamicMixingSelectionController(q);

        stateMachine = new QStateMachine(this);
        idleState = new QState;
        insertionState = new QState;
        moveState = new QState;
        deletionState = new QState;
        rubberBandState = new QState;
        stateMachine->addState(idleState);
        stateMachine->addState(insertionState);
        stateMachine->addState(moveState);
        stateMachine->addState(deletionState);
        stateMachine->addState(rubberBandState);
        stateMachine->setInitialState(idleState);
        idleState->addTransition(this, &DynamicMixingEditorContextPrivate::insertionStarted,
                                 insertionState);
        idleState->addTransition(this, &DynamicMixingEditorContextPrivate::moveStarted,
                                 moveState);
        idleState->addTransition(this, &DynamicMixingEditorContextPrivate::deletionStarted,
                                 deletionState);
        idleState->addTransition(this, &DynamicMixingEditorContextPrivate::rubberBandStarted,
                                 rubberBandState);
        insertionState->addTransition(this,
                                      &DynamicMixingEditorContextPrivate::operationFinished,
                                      idleState);
        moveState->addTransition(this, &DynamicMixingEditorContextPrivate::operationFinished,
                                 idleState);
        deletionState->addTransition(this,
                                     &DynamicMixingEditorContextPrivate::operationFinished,
                                     idleState);
        rubberBandState->addTransition(this,
                                       &DynamicMixingEditorContextPrivate::operationFinished,
                                       idleState);
        stateMachine->start();

        QObject::connect(dynamicSelectionModel,
                         &dspx::DynamicMixingAnchorSelectionModel::itemSelected,
                         q, [this](dspx::DynamicMixingAnchor *item, bool selected) {
            if (auto *viewItem = viewItemMap.value(item))
                viewItem->setSelected(selected);
        });

        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::anchorInsertionStarted,
                         q, [this] { beginOperation(Inserting); });
        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::anchorInsertionCommitted,
                         q, [this](QQuickItem *, sflow::DynamicMixingAnchorViewModel *item) {
            commitInsertion(item);
        });
        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::anchorInsertionAborted,
                         q, [this] { abortOperation(); });

        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::anchorMovingStarted,
                         q, [this](QQuickItem *, sflow::DynamicMixingAnchorViewModel *item) {
            if (!beginOperation(Moving))
                return;
            for (auto *object : viewSelectionController->getSelectedItems())
                operationItems.append(
                    qobject_cast<sflow::DynamicMixingAnchorViewModel *>(object));
            if (!operationItems.contains(item))
                operationItems.append(item);
        });
        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::anchorMovingCommitted,
                         q, [this] { commitMove(); });
        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::anchorMovingAborted,
                         q, [this] { abortOperation(); });

        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::anchorDeletionStarted,
                         q, [this](QQuickItem *, sflow::DynamicMixingAnchorViewModel *item) {
            if (!beginOperation(Deleting))
                return;
            auto selected = viewSelectionController->getSelectedItems();
            if (!item->isSelected())
                selected = {item};
            for (auto *object : selected) {
                auto *viewItem = qobject_cast<sflow::DynamicMixingAnchorViewModel *>(object);
                if (auto *documentItem = documentItemMap.value(viewItem))
                    deletionItems.append(documentItem);
            }
        });
        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::anchorDeletionCommitted,
                         q, [this] { commitDeletion(); });
        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::anchorDeletionAborted,
                         q, [this] { abortOperation(); });

        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::rubberBandDraggingStarted,
                         q, [this] {
            if (operation == Idle)
                operation = RubberBandSelecting;
            if (operation == RubberBandSelecting)
                Q_EMIT rubberBandStarted();
        });
        const auto finishRubberBand = [this] {
            if (operation == RubberBandSelecting)
                finishOperation();
        };
        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::rubberBandDraggingCommitted,
                         q, finishRubberBand);
        QObject::connect(interactionController,
                         &sflow::DynamicMixingEditorInteractionController::rubberBandDraggingAborted,
                         q, finishRubberBand);
    }

    void DynamicMixingEditorContextPrivate::disconnectTarget() {
        for (const auto &connection : std::as_const(targetConnections))
            QObject::disconnect(connection);
        targetConnections.clear();
    }

    void DynamicMixingEditorContextPrivate::bindTarget() {
        Q_Q(DynamicMixingEditorContext);
        disconnectTarget();
        sources = singingClip ? singingClip->sources() : nullptr;
        sequence = sources ? sources->dynamicMixingAnchors() : nullptr;

        if (singingClip) {
            targetConnections.append(QObject::connect(
                singingClip, &dspx::SingingClip::sourcesChanged, q,
                [this] {
                    if (operation != Idle)
                        abortOperation();
                    bindTarget();
                }));
        }
        if (sources) {
            targetConnections.append(QObject::connect(
                sources->singers(), &dspx::SingerList::itemsChanged, q,
                [this] { updateVoiceCount(); }));
        }
        if (sequence) {
            targetConnections.append(QObject::connect(
                sequence, &dspx::DynamicMixingAnchorSequence::itemInserted, q,
                [this](dspx::DynamicMixingAnchor *item) {
                    if (suppressDocumentSync)
                        return;
                    if (operation != Idle) {
                        rebuildPending = true;
                        return;
                    }
                    bindDocumentItem(item);
                }));
            targetConnections.append(QObject::connect(
                sequence, &dspx::DynamicMixingAnchorSequence::itemRemoved, q,
                [this](dspx::DynamicMixingAnchor *item) {
                    if (suppressDocumentSync)
                        return;
                    if (operation != Idle) {
                        rebuildPending = true;
                        return;
                    }
                    unbindDocumentItem(item);
                }));
        }
        rebuild();
    }

    void DynamicMixingEditorContextPrivate::clearViewItems() {
        const auto items = viewModel->items();
        for (auto it = viewItemMap.cbegin(); it != viewItemMap.cend(); ++it) {
            QObject::disconnect(it.key(), nullptr, it.value(), nullptr);
            QObject::disconnect(it.value(), nullptr, q_ptr, nullptr);
        }
        viewItemMap.clear();
        documentItemMap.clear();
        for (auto *object : items) {
            auto *viewItem = qobject_cast<sflow::DynamicMixingAnchorViewModel *>(object);
            if (!viewItem)
                continue;
            viewModel->removeItem(viewItem);
            viewItem->deleteLater();
        }
    }

    void DynamicMixingEditorContextPrivate::rebuild() {
        clearViewItems();
        updateVoiceCount();
        if (sequence) {
            for (auto *item : sequence->asRange())
                bindDocumentItem(item);
        }
        syncSelection();
        Q_EMIT viewSelectionController->currentItemChanged();
        Q_EMIT viewSelectionController->editScopeFocusedChanged();
        rebuildPending = false;
    }

    void DynamicMixingEditorContextPrivate::bindDocumentItem(
        dspx::DynamicMixingAnchor *item,
        sflow::DynamicMixingAnchorViewModel *existingViewItem) {
        Q_Q(DynamicMixingEditorContext);
        if (!item || viewItemMap.contains(item))
            return;
        auto *viewItem = existingViewItem
                             ? existingViewItem
                             : new sflow::DynamicMixingAnchorViewModel(viewModel);
        viewItem->setPosition(item->position());
        viewItem->setRatio(item->ratio());
        viewItemMap.insert(item, viewItem);
        documentItemMap.insert(viewItem, item);
        if (!viewModel->contains(viewItem))
            viewModel->insertItem(viewItem);
        viewItem->setSelected(dynamicSelectionModel->isItemSelected(item));

        QObject::connect(item, &dspx::DynamicMixingAnchor::positionChanged,
                         viewItem, [this, item, viewItem] {
            if (suppressDocumentSync)
                return;
            if (operation != Idle) {
                rebuildPending = true;
                return;
            }
            if (viewItem->position() != item->position())
                viewModel->moveItem(viewItem, item->position());
        });
        QObject::connect(item, &dspx::DynamicMixingAnchor::ratioChanged,
                         viewItem, [this, item, viewItem] {
            if (suppressDocumentSync)
                return;
            if (operation != Idle) {
                rebuildPending = true;
                return;
            }
            if (viewItem->ratio() != item->ratio())
                viewItem->setRatio(item->ratio());
        });
        QObject::connect(viewItem, &sflow::DynamicMixingAnchorViewModel::positionChanged,
                         q, [this, item, viewItem] {
            if (operation == Idle && viewItem->position() != item->position())
                viewModel->moveItem(viewItem, item->position());
        });
        QObject::connect(viewItem, &sflow::DynamicMixingAnchorViewModel::ratioChanged,
                         q, [this, item, viewItem] {
            if (operation == Idle && viewItem->ratio() != item->ratio())
                viewItem->setRatio(item->ratio());
        });
    }

    void DynamicMixingEditorContextPrivate::unbindDocumentItem(
        dspx::DynamicMixingAnchor *item) {
        if (auto *viewItem = viewItemMap.take(item)) {
            documentItemMap.remove(viewItem);
            QObject::disconnect(item, nullptr, viewItem, nullptr);
            QObject::disconnect(viewItem, nullptr, q_ptr, nullptr);
            viewModel->removeItem(viewItem);
            viewItem->deleteLater();
        }
    }

    void DynamicMixingEditorContextPrivate::updateVoiceCount() {
        Q_Q(DynamicMixingEditorContext);
        const bool oldAvailable = available;
        const int count = sources ? sources->singers()->size() : 0;
        viewModel->setVoiceCount(std::max(1, count));
        const bool newAvailable = count > 0;
        available = newAvailable;
        if (oldAvailable != newAvailable)
            Q_EMIT q->availableChanged();
    }

    void DynamicMixingEditorContextPrivate::syncSelection() {
        for (auto it = viewItemMap.cbegin(); it != viewItemMap.cend(); ++it)
            it.value()->setSelected(dynamicSelectionModel->isItemSelected(it.key()));
    }

    void DynamicMixingEditorContextPrivate::restoreOperationSelection() {
        if (!restoreSelectionOnAbort)
            return;
        selectionModel->select(nullptr, dspx::SelectionModel::ClearPreviousSelection,
                               dspx::SelectionModel::ST_DynamicMixingAnchor, sequence);
        for (auto *item : std::as_const(selectionBeforeOperation)) {
            if (item && sequence && sequence->contains(item))
                selectionModel->select(item, dspx::SelectionModel::Select);
        }
        if (currentItemBeforeOperation && sequence
            && sequence->contains(currentItemBeforeOperation)) {
            selectionModel->select(currentItemBeforeOperation,
                                   dspx::SelectionModel::SetCurrentItem);
        }
    }

    bool DynamicMixingEditorContextPrivate::beginOperation(Operation nextOperation) {
        if (operation != Idle || !sources || sources->singers()->size() <= 0)
            return false;
        operation = nextOperation;
        operationItems.clear();
        deletionItems.clear();
        restoreSelectionOnAbort = selectionModel->selectionType()
                                  == dspx::SelectionModel::ST_DynamicMixingAnchor
                                  && dynamicSelectionModel
                                         ->dynamicMixingAnchorSequenceWithSelectedItems()
                                         == sequence;
        selectionBeforeOperation = restoreSelectionOnAbort
                                       ? dynamicSelectionModel->selectedItems()
                                       : QList<dspx::DynamicMixingAnchor *>{};
        currentItemBeforeOperation = restoreSelectionOnAbort
                                         ? dynamicSelectionModel->currentItem()
                                         : nullptr;
        transactionId = document->transactionController()->beginTransaction();
        if (transactionId == Core::TransactionController::TransactionId::Invalid) {
            operation = Idle;
            rebuildPending = true;
            return false;
        }
        switch (nextOperation) {
            case Inserting:
                Q_EMIT insertionStarted();
                break;
            case Moving:
                Q_EMIT moveStarted();
                break;
            case Deleting:
                Q_EMIT deletionStarted();
                break;
            case Idle:
            case RubberBandSelecting:
                break;
        }
        return true;
    }

    void DynamicMixingEditorContextPrivate::finishOperation() {
        Q_EMIT operationFinished();
        operation = Idle;
        operationItems.clear();
        deletionItems.clear();
        selectionBeforeOperation.clear();
        currentItemBeforeOperation = nullptr;
        restoreSelectionOnAbort = false;
        if (rebuildPending)
            rebuild();
        else
            syncSelection();
    }

    void DynamicMixingEditorContextPrivate::abortOperation() {
        if (transactionId != Core::TransactionController::TransactionId::Invalid)
            document->transactionController()->abortTransaction(transactionId);
        transactionId = {};
        if (operation != Idle)
            Q_EMIT operationFinished();
        operation = Idle;
        restoreOperationSelection();
        operationItems.clear();
        deletionItems.clear();
        selectionBeforeOperation.clear();
        currentItemBeforeOperation = nullptr;
        restoreSelectionOnAbort = false;
        rebuild();
    }

    void DynamicMixingEditorContextPrivate::commitInsertion(
        sflow::DynamicMixingAnchorViewModel *viewItem) {
        if (operation != Inserting
            || transactionId == Core::TransactionController::TransactionId::Invalid
            || !viewItem || !sequence) {
            abortOperation();
            return;
        }
        auto *item = document->model()->createDynamicMixingAnchor();
        item->setPosition(viewItem->position());
        item->setRatio(viewItem->ratio());
        suppressDocumentSync = true;
        const bool inserted = sequence->insertItem(item);
        suppressDocumentSync = false;
        if (!inserted) {
            document->model()->destroyItem(item);
            abortOperation();
            return;
        }
        bindDocumentItem(item, viewItem);
        document->transactionController()->commitTransaction(
            transactionId, DynamicMixingEditorContext::tr("Adding voice blending anchor"));
        transactionId = {};
        finishOperation();
    }

    void DynamicMixingEditorContextPrivate::commitMove() {
        struct Entry {
            dspx::DynamicMixingAnchor *documentItem;
            sflow::DynamicMixingAnchorViewModel *viewItem;
            int position;
            QList<double> ratio;
        };
        if (operation != Moving
            || transactionId == Core::TransactionController::TransactionId::Invalid) {
            abortOperation();
            return;
        }
        QList<Entry> entries;
        QSet<int> targetPositions;
        QSet<dspx::DynamicMixingAnchor *> movedItems;
        bool changed = false;
        for (auto *viewItem : std::as_const(operationItems)) {
            auto *item = documentItemMap.value(viewItem);
            if (!item || targetPositions.contains(viewItem->position())) {
                abortOperation();
                return;
            }
            targetPositions.insert(viewItem->position());
            movedItems.insert(item);
            entries.append({item, viewItem, viewItem->position(), viewItem->ratio()});
            changed = changed || item->position() != viewItem->position()
                      || item->ratio() != viewItem->ratio();
        }
        if (!changed) {
            abortOperation();
            return;
        }

        QSet<dspx::DynamicMixingAnchor *> overlaps;
        for (const auto &entry : std::as_const(entries)) {
            for (auto *item : sequence->slice(entry.position, 1)) {
                if (!movedItems.contains(item))
                    overlaps.insert(item);
            }
        }

        suppressDocumentSync = true;
        for (auto *item : std::as_const(overlaps)) {
            sequence->removeItem(item);
            unbindDocumentItem(item);
            document->model()->destroyItem(item);
        }
        for (const auto &entry : std::as_const(entries)) {
            if (entry.documentItem->position() != entry.position)
                sequence->removeItem(entry.documentItem);
        }
        for (const auto &entry : std::as_const(entries)) {
            entry.documentItem->setRatio(entry.ratio);
            if (entry.documentItem->position() != entry.position) {
                entry.documentItem->setPosition(entry.position);
                if (!sequence->insertItem(entry.documentItem)) {
                    suppressDocumentSync = false;
                    abortOperation();
                    return;
                }
            }
        }
        suppressDocumentSync = false;
        restoreOperationSelection();
        document->transactionController()->commitTransaction(
            transactionId, DynamicMixingEditorContext::tr("Editing voice blending"));
        transactionId = {};
        finishOperation();
    }

    void DynamicMixingEditorContextPrivate::commitDeletion() {
        if (operation != Deleting
            || transactionId == Core::TransactionController::TransactionId::Invalid) {
            abortOperation();
            return;
        }
        suppressDocumentSync = true;
        for (auto *item : std::as_const(deletionItems)) {
            if (!sequence || !sequence->contains(item))
                continue;
            sequence->removeItem(item);
            unbindDocumentItem(item);
            document->model()->destroyItem(item);
        }
        suppressDocumentSync = false;
        document->transactionController()->commitTransaction(
            transactionId, DynamicMixingEditorContext::tr("Deleting voice blending anchor"));
        transactionId = {};
        finishOperation();
    }

    DynamicMixingEditorContext::DynamicMixingEditorContext(
        ProjectViewModelContext *projectContext)
        : QObject(projectContext), d_ptr(new DynamicMixingEditorContextPrivate) {
        Q_D(DynamicMixingEditorContext);
        d->q_ptr = this;
        d->projectContext = projectContext;
        d->initialize();
    }

    DynamicMixingEditorContext::~DynamicMixingEditorContext() = default;

    dspx::SingingClip *DynamicMixingEditorContext::singingClip() const {
        Q_D(const DynamicMixingEditorContext);
        return d->singingClip;
    }

    bool DynamicMixingEditorContext::isAvailable() const {
        Q_D(const DynamicMixingEditorContext);
        return d->available;
    }

    sflow::DynamicMixingViewModel *DynamicMixingEditorContext::dynamicMixingViewModel() const {
        Q_D(const DynamicMixingEditorContext);
        return d->viewModel;
    }

    sflow::SelectionController *DynamicMixingEditorContext::selectionController() const {
        Q_D(const DynamicMixingEditorContext);
        return d->viewSelectionController;
    }

    sflow::DynamicMixingEditorInteractionController *
    DynamicMixingEditorContext::interactionController() const {
        Q_D(const DynamicMixingEditorContext);
        return d->interactionController;
    }

    dspx::DynamicMixingAnchor *DynamicMixingEditorContext::getDocumentItemFromViewItem(
        sflow::DynamicMixingAnchorViewModel *viewItem) const {
        Q_D(const DynamicMixingEditorContext);
        return d->documentItemMap.value(viewItem);
    }

    sflow::DynamicMixingAnchorViewModel *
    DynamicMixingEditorContext::getViewItemFromDocumentItem(
        dspx::DynamicMixingAnchor *item) const {
        Q_D(const DynamicMixingEditorContext);
        return d->viewItemMap.value(item);
    }

    void DynamicMixingEditorContext::setSingingClip(dspx::SingingClip *singingClip) {
        Q_D(DynamicMixingEditorContext);
        if (d->singingClip == singingClip)
            return;
        if (d->operation != DynamicMixingEditorContextPrivate::Idle)
            d->abortOperation();
        d->singingClip = singingClip;
        d->bindTarget();
        Q_EMIT singingClipChanged();
    }

}

#include "moc_DynamicMixingEditorContext.cpp"
