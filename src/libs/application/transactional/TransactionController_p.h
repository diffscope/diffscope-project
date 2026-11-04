// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_P_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_P_H

#include <transactional/TransactionController.h>

#include <optional>

#include <QSet>

class QStandardItemModel;
class QTimer;

namespace Core {

    class TransactionControllerPrivate {
        Q_DECLARE_PUBLIC(TransactionController)
    public:
        using TransactionId = TransactionController::TransactionId;

        class WriteTransition {
        public:
            explicit WriteTransition(TransactionControllerPrivate *controller);
            ~WriteTransition();

        private:
            Q_DISABLE_COPY_MOVE(WriteTransition)
            TransactionControllerPrivate *m_controller;
        };

        TransactionController *q_ptr = nullptr;
        TransactionalStrategy *strategy = nullptr;

        TransactionId currentTransactionId = TransactionId::Invalid;
        bool autoCommit = false;
        bool transitioning = false;
        int *readLockIdCounter = nullptr;
        QSet<TransactionController::ReadLockId> readLocks;
        int transactionIdCounter = 0;

        int currentStep = 0;
        int cleanStep = 0;
        QStandardItemModel *stepModel = nullptr;

        QTimer *autoCommitTimer = nullptr;
        QString pendingTransactionName;

        TransactionId beginTransaction(std::optional<int> timeoutMs, const QString &name);
        void autoCommitTransaction();
        void abortCurrentTransaction();
        void commitCurrentTransaction(const QString &name);
        void finishTransaction();
    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_P_H
