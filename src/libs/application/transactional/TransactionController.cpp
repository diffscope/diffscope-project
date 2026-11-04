// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "TransactionController.h"
#include "TransactionController_p.h"

#include <exception>
#include <limits>

#include <QDebug>
#include <QMetaObject>
#include <QStandardItemModel>
#include <QTimer>

#include <transactional/TransactionalStrategy.h>

namespace Core {

    TransactionController::TransactionController(TransactionalStrategy *strategy, QObject *parent)
        : QObject(parent), d_ptr(new TransactionControllerPrivate) {
        Q_D(TransactionController);
        // Retain the counter of the module that constructs the controller.
        static int readLockIdCounter = 0;
        d->readLockIdCounter = &readLockIdCounter;
        d->q_ptr = this;
        d->strategy = strategy;
        d->stepModel = new QStandardItemModel(this);
        d->autoCommitTimer = new QTimer(this);
        d->autoCommitTimer->setSingleShot(true);
        connect(d->autoCommitTimer, &QTimer::timeout, this, [d] {
            if (d->transitioning || !d->autoCommit)
                return;
            const TransactionControllerPrivate::WriteTransition transition(d);
            d->autoCommitTransaction();
        });
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

    bool TransactionController::hasReadLocks() const {
        Q_D(const TransactionController);
        return !d->readLocks.isEmpty();
    }

    bool TransactionController::canBeginTransaction() const {
        Q_D(const TransactionController);
        return !d->transitioning && !hasReadLocks() && (!isTransactionActive() || d->autoCommit);
    }

    bool TransactionController::canAcquireReadLock() const {
        Q_D(const TransactionController);
        return !d->transitioning && !isTransactionActive();
    }

    bool TransactionController::canMoveCurrentStep() const {
        return canBeginTransaction();
    }

    TransactionController::ReadLockId TransactionController::tryAcquireReadLock() {
        Q_D(TransactionController);
        auto &lastReadLockId = *d->readLockIdCounter;
        if (!canAcquireReadLock() || lastReadLockId == std::numeric_limits<int>::max())
            return ReadLockId::Invalid;

        const auto readLockId = static_cast<ReadLockId>(++lastReadLockId);
        d->readLocks.insert(readLockId);
        if (d->readLocks.size() == 1)
            Q_EMIT accessStateChanged();
        return readLockId;
    }

    bool TransactionController::releaseReadLock(ReadLockId readLockId) {
        Q_D(TransactionController);
        if (!d->readLocks.remove(readLockId))
            return false;
        if (d->readLocks.isEmpty())
            Q_EMIT accessStateChanged();
        return true;
    }

    TransactionController::ScopedReadLock::ScopedReadLock(TransactionController *controller) {
        acquire(controller);
    }

    TransactionController::ScopedReadLock::~ScopedReadLock() {
        release();
    }

    bool TransactionController::ScopedReadLock::acquire(TransactionController *controller) {
        if (m_controller)
            return m_controller.data() == controller;
        m_controller = controller;
        m_id = controller ? controller->tryAcquireReadLock() : ReadLockId::Invalid;
        if (m_id == ReadLockId::Invalid)
            m_controller.clear();
        return !m_controller.isNull();
    }

    void TransactionController::ScopedReadLock::release() {
        const auto controller = m_controller;
        const auto id = m_id;
        m_controller.clear();
        m_id = ReadLockId::Invalid;
        if (controller)
            controller->releaseReadLock(id);
    }

    TransactionController::ScopedReadLock::operator bool() const {
        return !m_controller.isNull();
    }

    bool TransactionController::withWriteAccess(const std::function<void()> &callback) {
        Q_D(TransactionController);
        if (isTransactionActive() || !canBeginTransaction())
            return false;
        const TransactionControllerPrivate::WriteTransition transition(d);
        callback();
        return true;
    }

    int TransactionController::currentStep() const {
        Q_D(const TransactionController);
        return d->currentStep;
    }

    void TransactionController::setCurrentStep(int step) {
        Q_D(TransactionController);
        if (!canMoveCurrentStep() || step < 0 || step > totalSteps())
            return;
        if (d->currentStep == step && !d->autoCommit)
            return;

        const TransactionControllerPrivate::WriteTransition transition(d);
        d->autoCommitTransaction();
        if (step > totalSteps() || step == d->currentStep)
            return;

        d->strategy->moveCurrentStepBy(step - d->currentStep);
        d->currentStep = step;
        Q_EMIT currentStepChanged(step);
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
        if (step > totalSteps() || d->cleanStep == step)
            return;
        d->cleanStep = step;
        Q_EMIT cleanStepChanged(step);
    }

    QString TransactionController::stepName(int step) const {
        Q_D(const TransactionController);
        if (step < 0 || step >= d->stepModel->rowCount())
            return {};
        auto item = d->stepModel->item(step);
        return item ? item->text() : QString();
    }

    QAbstractItemModel *TransactionController::stepModel() const {
        Q_D(const TransactionController);
        return d->stepModel;
    }

    TransactionController::TransactionId TransactionController::beginTransaction() {
        Q_D(TransactionController);
        return d->beginTransaction(std::nullopt, {});
    }

    bool TransactionController::abortTransaction(TransactionId transactionId) {
        Q_D(TransactionController);
        if (d->transitioning || transactionId == TransactionId::Invalid || d->currentTransactionId != transactionId)
            return false;

        const TransactionControllerPrivate::WriteTransition transition(d);
        d->abortCurrentTransaction();
        return true;
    }

    bool TransactionController::commitTransaction(TransactionId transactionId, const QString &name) {
        Q_D(TransactionController);
        if (d->transitioning || transactionId == TransactionId::Invalid || d->currentTransactionId != transactionId)
            return false;

        const TransactionControllerPrivate::WriteTransition transition(d);
        d->commitCurrentTransaction(name);
        return true;
    }

    TransactionController::TransactionId TransactionController::beginAutoCommitTransaction(int timeoutMs, const QString &name) {
        Q_D(TransactionController);
        return d->beginTransaction(timeoutMs, name);
    }

    void TransactionController::restartAutoCommitTimer(TransactionId transactionId, int timeoutMs, const QString &name) {
        Q_D(TransactionController);
        if (d->transitioning || !d->autoCommit || d->currentTransactionId != transactionId)
            return;
        if (!name.isEmpty())
            d->pendingTransactionName = name;
        d->autoCommitTimer->start(timeoutMs);
    }

    TransactionControllerPrivate::WriteTransition::WriteTransition(TransactionControllerPrivate *controller)
        : m_controller(controller) {
        m_controller->transitioning = true;
        Q_EMIT m_controller->q_ptr->accessStateChanged();
    }

    TransactionControllerPrivate::WriteTransition::~WriteTransition() {
        m_controller->transitioning = false;
        auto *controller = m_controller->q_ptr;
        if (m_controller->autoCommit) {
            // Defer notification until the caller can receive the transaction identifier.
            QMetaObject::invokeMethod(controller, &TransactionController::accessStateChanged, Qt::QueuedConnection);
        } else {
            Q_EMIT controller->accessStateChanged();
        }
    }

    TransactionController::TransactionId TransactionControllerPrivate::beginTransaction(std::optional<int> timeoutMs, const QString &name) {
        Q_Q(TransactionController);
        if (!q->canBeginTransaction())
            return TransactionId::Invalid;

        const WriteTransition transition(this);
        autoCommitTransaction();
        if (!strategy->beginTransaction())
            return TransactionId::Invalid;

        const auto transactionId = static_cast<TransactionId>(++transactionIdCounter);
        currentTransactionId = transactionId;
        autoCommit = timeoutMs.has_value();
        pendingTransactionName = name;
        Q_EMIT q->transactionActiveChanged(true);
        Q_EMIT q->transactionBegun();
        if (timeoutMs)
            autoCommitTimer->start(*timeoutMs);
        return transactionId;
    }

    void TransactionControllerPrivate::autoCommitTransaction() {
        Q_Q(TransactionController);
        if (!autoCommit)
            return;

        const auto transactionId = currentTransactionId;
        const auto name = pendingTransactionName;
        try {
            commitCurrentTransaction(name);
        } catch (const std::exception &error) {
            qWarning() << "The transaction could not be committed automatically." << error.what();
            abortCurrentTransaction();
            return;
        }
        Q_EMIT q->transactionAutoCommitted(transactionId);
    }

    void TransactionControllerPrivate::abortCurrentTransaction() {
        Q_Q(TransactionController);
        strategy->abortTransaction();
        finishTransaction();
        Q_EMIT q->transactionAborted();
    }

    void TransactionControllerPrivate::commitCurrentTransaction(const QString &name) {
        Q_Q(TransactionController);
        const bool undoStepCreated = strategy->commitTransaction();
        bool cleanStepChanged = false;
        if (undoStepCreated) {
            const int totalRows = stepModel->rowCount();
            if (currentStep < totalRows)
                stepModel->removeRows(currentStep, totalRows - currentStep);
            stepModel->appendRow(new QStandardItem(name));
            ++currentStep;
            if (cleanStep >= currentStep) {
                cleanStep = -1;
                cleanStepChanged = true;
            }
        }

        finishTransaction();
        if (cleanStepChanged)
            Q_EMIT q->cleanStepChanged(cleanStep);
        if (undoStepCreated) {
            Q_EMIT q->currentStepChanged(currentStep);
            Q_EMIT q->totalStepsChanged(q->totalSteps());
        }
        Q_EMIT q->transactionCommitted(name);
    }

    void TransactionControllerPrivate::finishTransaction() {
        Q_Q(TransactionController);
        autoCommitTimer->stop();
        currentTransactionId = TransactionId::Invalid;
        autoCommit = false;
        pendingTransactionName.clear();
        Q_EMIT q->transactionActiveChanged(false);
    }

}

#include "moc_TransactionController.cpp"
