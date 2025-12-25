#include "TransactionController.h"
#include "TransactionController_p.h"

#include <transactional/TransactionalStrategy.h>

namespace Core {

    TransactionController::TransactionController(TransactionalStrategy *strategy, QObject *parent)
        : QObject(parent), d_ptr(new TransactionControllerPrivate) {
        Q_D(TransactionController);
        d->q_ptr = this;
        d->strategy = strategy;
        d->init();
    }

    TransactionController::~TransactionController() = default;

    TransactionalStrategy *TransactionController::strategy() const {
        Q_D(const TransactionController);
        return d->strategy;
    }

    bool TransactionController::isTransactionActive() const {
        Q_D(const TransactionController);
        return d->currentTransactionId != TransactionId::Invalid;
    }

    bool TransactionController::isExclusiveToTransaction() const {
        Q_D(const TransactionController);
        return isTransactionActive() && d->isExclusive;
    }

    int TransactionController::currentStep() const {
        Q_D(const TransactionController);
        return d->currentStep;
    }

    void TransactionController::setCurrentStep(int step) {
        Q_D(TransactionController);
        
        // Validate range
        if (step < 0 || step > totalSteps()) {
            return;
        }
        
        // If there's a non-exclusive transaction, auto-commit it
        if (isTransactionActive() && !d->isExclusive) {
            d->autoCommitNonExclusiveTransaction();
        }
        
        // If exclusive transaction is active, do nothing
        if (isExclusiveToTransaction()) {
            return;
        }
        
        if (d->currentStep != step) {
            // Calculate delta before updating currentStep
            int delta = step - d->currentStep;
            
            d->currentStep = step;
            
            // Move the strategy to the target step
            if (delta != 0) {
                d->strategy->moveCurrentStepBy(delta);
            }
            
            Q_EMIT currentStepChanged(d->currentStep);
        }
    }

    int TransactionController::totalSteps() const {
        Q_D(const TransactionController);
        return d->stepModel->rowCount();
    }

    int TransactionController::cleanStep() const {
        Q_D(const TransactionController);
        return d->cleanStep;
    }

    void TransactionController::setCleanStep(int step) {
        Q_D(TransactionController);
        
        // Validate range
        if (step > totalSteps()) {
            return;
        }
        
        if (d->cleanStep != step) {
            d->cleanStep = step;
            Q_EMIT cleanStepChanged(d->cleanStep);
        }
    }

    QString TransactionController::stepName(int step) const {
        Q_D(const TransactionController);
        if (step < 0 || step >= d->stepModel->rowCount()) {
            return QString();
        }
        auto item = d->stepModel->item(step);
        return item ? item->text() : QString();
    }

    QAbstractItemModel *TransactionController::stepModel() const {
        Q_D(const TransactionController);
        return d->stepModel;
    }

    TransactionController::TransactionId TransactionController::beginTransaction() {
        Q_D(TransactionController);
        
        // If exclusive transaction is active, return invalid
        if (isExclusiveToTransaction()) {
            return TransactionId::Invalid;
        }
        
        // If non-exclusive transaction is active, auto-commit it
        if (isTransactionActive()) {
            d->autoCommitNonExclusiveTransaction();
        }
        
        // Generate new transaction ID and start exclusive transaction
        auto transactionId = d->generateTransactionId();
        d->currentTransactionId = transactionId;
        d->isExclusive = true;
        
        d->strategy->beginTransaction();
        
        Q_EMIT transactionActiveChanged(true);
        Q_EMIT exclusiveToTransactionChanged(true);
        Q_EMIT transactionBegun();
        
        return transactionId;
    }

    bool TransactionController::abortTransaction(TransactionId transactionId) {
        Q_D(TransactionController);
        
        // Validate transaction ID
        if (transactionId == TransactionId::Invalid || d->currentTransactionId != transactionId) {
            return false;
        }
        
        // Stop auto-commit timer if active
        if (d->autoCommitTimer && d->autoCommitTimer->isActive()) {
            d->autoCommitTimer->stop();
        }
        
        d->strategy->abortTransaction();
        
        bool wasExclusive = d->isExclusive;
        d->currentTransactionId = TransactionId::Invalid;
        d->isExclusive = false;
        d->pendingTransactionName.clear();
        
        Q_EMIT transactionActiveChanged(false);
        if (wasExclusive) {
            Q_EMIT exclusiveToTransactionChanged(false);
        }
        Q_EMIT transactionAborted();
        
        return true;
    }

    bool TransactionController::commitTransaction(TransactionId transactionId, const QString &name) {
        Q_D(TransactionController);
        
        // Validate transaction ID
        if (transactionId == TransactionId::Invalid || d->currentTransactionId != transactionId) {
            return false;
        }
        
        d->commitCurrentTransaction(name);
        return true;
    }

    TransactionController::TransactionId TransactionController::beginNonExclusiveTransaction(int lifeTime, const QString &name) {
        Q_D(TransactionController);
        
        // If exclusive transaction is active, return invalid
        if (isExclusiveToTransaction()) {
            return TransactionId::Invalid;
        }
        
        // If non-exclusive transaction is active, auto-commit it
        if (isTransactionActive()) {
            d->autoCommitNonExclusiveTransaction();
        }
        
        // Generate new transaction ID and start non-exclusive transaction
        auto transactionId = d->generateTransactionId();
        d->currentTransactionId = transactionId;
        d->isExclusive = false;
        d->pendingTransactionName = name;
        
        d->strategy->beginTransaction();
        
        // Start auto-commit timer
        if (!d->autoCommitTimer) {
            d->autoCommitTimer = new QTimer(this);
            d->autoCommitTimer->setSingleShot(true);
            connect(d->autoCommitTimer, &QTimer::timeout, this, [d]() {
                d->autoCommitNonExclusiveTransaction();
            });
        }
        d->autoCommitTimer->start(lifeTime);
        
        Q_EMIT transactionActiveChanged(true);
        Q_EMIT transactionBegun();
        
        return transactionId;
    }

    void TransactionController::resetNonExclusiveTransactionLifeTime(TransactionId transactionId, int lifeTime, const QString &name) {
        Q_D(TransactionController);
        
        // Validate transaction ID and ensure it's non-exclusive
        if (transactionId == TransactionId::Invalid || 
            d->currentTransactionId != transactionId || 
            d->isExclusive) {
            return;
        }
        
        // Update name if provided
        if (!name.isEmpty()) {
            d->pendingTransactionName = name;
        }
        
        // Reset timer
        if (d->autoCommitTimer && d->autoCommitTimer->isActive()) {
            d->autoCommitTimer->start(lifeTime);
        }
    }

    // Private implementation methods
    void TransactionControllerPrivate::init() {
        stepModel = new QStandardItemModel(q_ptr);
    }



    void TransactionControllerPrivate::autoCommitNonExclusiveTransaction() {
        Q_Q(TransactionController);
        
        if (currentTransactionId == TransactionController::TransactionId::Invalid || isExclusive) {
            return;
        }
        
        auto transactionId = currentTransactionId;
        commitCurrentTransaction(pendingTransactionName);
        
        Q_EMIT q->nonExclusiveTransactionAutoCommitted(transactionId);
    }

    TransactionController::TransactionId TransactionControllerPrivate::generateTransactionId() {
        return static_cast<TransactionController::TransactionId>(++transactionIdCounter);
    }

    void TransactionControllerPrivate::commitCurrentTransaction(const QString &name) {
        Q_Q(TransactionController);
        
        // Stop auto-commit timer if active
        if (autoCommitTimer && autoCommitTimer->isActive()) {
            autoCommitTimer->stop();
        }
        
        strategy->commitTransaction();
        
        // Handle undo/redo logic: if we're not at the end of the step list,
        // remove everything after current position (this happens when committing
        // after an undo operation)
        int totalRows = stepModel->rowCount();
        if (currentStep < totalRows) {
            stepModel->removeRows(currentStep, totalRows - currentStep);
        }
        
        // Add new step
        auto item = new QStandardItem(name);
        stepModel->appendRow(item);
        currentStep++;
        
        bool wasExclusive = isExclusive;
        currentTransactionId = TransactionController::TransactionId::Invalid;
        isExclusive = false;
        pendingTransactionName.clear();
        
        Q_EMIT q->transactionActiveChanged(false);
        if (wasExclusive) {
            Q_EMIT q->exclusiveToTransactionChanged(false);
        }
        if (cleanStep >= currentStep) {
            cleanStep = -1;
            Q_EMIT q->cleanStepChanged(cleanStep);
        }
        Q_EMIT q->currentStepChanged(currentStep);
        Q_EMIT q->totalStepsChanged(q->totalSteps());
        Q_EMIT q->transactionCommitted(name);
    }

}

#include "moc_TransactionController.cpp"