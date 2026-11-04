// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_H

#include <concepts>
#include <functional>

#include <QObject>
#include <QPointer>
#include <QScopedPointer>

class QAbstractItemModel;

namespace Core {

    class TransactionalStrategy;

    class TransactionControllerPrivate;

    class TransactionController : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(TransactionController)
        Q_PROPERTY(bool transactionActive READ isTransactionActive NOTIFY transactionActiveChanged)
        Q_PROPERTY(bool hasReadLocks READ hasReadLocks NOTIFY accessStateChanged)
        Q_PROPERTY(bool canBeginTransaction READ canBeginTransaction NOTIFY accessStateChanged)
        Q_PROPERTY(bool canAcquireReadLock READ canAcquireReadLock NOTIFY accessStateChanged)
        Q_PROPERTY(bool canMoveCurrentStep READ canMoveCurrentStep NOTIFY accessStateChanged)
        Q_PROPERTY(int currentStep READ currentStep WRITE setCurrentStep NOTIFY currentStepChanged)
        Q_PROPERTY(int totalSteps READ totalSteps NOTIFY totalStepsChanged)
        Q_PROPERTY(int cleanStep READ cleanStep WRITE setCleanStep NOTIFY cleanStepChanged)
        Q_PROPERTY(QAbstractItemModel *stepModel READ stepModel CONSTANT)
    public:
        explicit TransactionController(TransactionalStrategy *strategy, QObject *parent = nullptr);
        ~TransactionController() override;

        TransactionalStrategy *strategy() const;

        bool isTransactionActive() const;
        bool hasReadLocks() const;
        bool canBeginTransaction() const;
        bool canAcquireReadLock() const;
        bool canMoveCurrentStep() const;

        enum class ReadLockId {
            Invalid = 0,
        };
        Q_ENUM(ReadLockId)

        Q_INVOKABLE ReadLockId tryAcquireReadLock();
        Q_INVOKABLE bool releaseReadLock(ReadLockId readLockId);

        class ScopedReadLock {
        public:
            ScopedReadLock() = default;
            explicit ScopedReadLock(TransactionController *controller);
            ~ScopedReadLock();

            bool acquire(TransactionController *controller);
            void release();
            explicit operator bool() const;

        private:
            Q_DISABLE_COPY_MOVE(ScopedReadLock)
            QPointer<TransactionController> m_controller;
            ReadLockId m_id = ReadLockId::Invalid;
        };

        bool withWriteAccess(const std::function<void()> &callback);

        int currentStep() const;
        void setCurrentStep(int step);

        int totalSteps() const;

        int cleanStep() const;
        void setCleanStep(int step);

        Q_INVOKABLE QString stepName(int step) const;
        QAbstractItemModel *stepModel() const;

        enum class TransactionId {
            Invalid = 0,
        };
        Q_ENUM(TransactionId)

        Q_INVOKABLE TransactionId beginTransaction();
        Q_INVOKABLE bool abortTransaction(TransactionId transactionId);
        Q_INVOKABLE bool commitTransaction(TransactionId transactionId, const QString &name);

        template <typename Callback>
        requires requires (Callback callback) {
            { callback() } -> std::convertible_to<bool>;
        }
        bool beginScopedTransaction(const QString &name, Callback callback) {
            auto transactionId = beginTransaction();
            if (transactionId == TransactionId::Invalid) {
                return false;
            }
            bool ok;
            if (!callback()) {
                ok = abortTransaction(transactionId);
            } else {
                ok = commitTransaction(transactionId, name);
            }
            Q_ASSERT(ok);
            return true;
        }

        template <typename Callback, typename CallbackIfFails>
        requires requires (Callback callback, CallbackIfFails callbackIfFails) {
            { callback() } -> std::convertible_to<bool>;
            { callbackIfFails() };
        }
        bool beginScopedTransaction(const QString &name, Callback callback, CallbackIfFails callbackIfFails) {
            if (!beginScopedTransaction(name, callback)) {
                callbackIfFails();
                return false;
            }
            return true;
        }

        Q_INVOKABLE TransactionId beginAutoCommitTransaction(int timeoutMs, const QString &name);
        Q_INVOKABLE void restartAutoCommitTimer(TransactionId transactionId, int timeoutMs, const QString &name = {});

    Q_SIGNALS:
        void transactionActiveChanged(bool isTransactionActive);
        void accessStateChanged();
        void currentStepChanged(int currentStep);
        void totalStepsChanged(int totalSteps);
        void cleanStepChanged(int cleanStep);

        void transactionBegun();
        void transactionAborted();
        void transactionCommitted(const QString &name);
        void transactionAutoCommitted(TransactionId transactionId);

    private:
        QScopedPointer<TransactionControllerPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_H
