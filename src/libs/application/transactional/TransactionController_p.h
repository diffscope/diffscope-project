#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_P_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_P_H

#include <QTimer>
#include <QStandardItemModel>
#include <QStringList>

#include <transactional/TransactionController.h>

namespace Core {

    class TransactionControllerPrivate {
        Q_DECLARE_PUBLIC(TransactionController)
    public:
        TransactionController *q_ptr;

        TransactionalStrategy *strategy;
        
        // Transaction state management
        TransactionController::TransactionId currentTransactionId = TransactionController::TransactionId::Invalid;
        bool isExclusive = false;
        int transactionIdCounter = 0;
        
        // Step management
        int currentStep = 0;
        int cleanStep = 0;
        QStandardItemModel *stepModel = nullptr;
        
        // Non-exclusive transaction timeout management
        QTimer *autoCommitTimer = nullptr;
        QString pendingTransactionName;
        
        void init();
        void autoCommitNonExclusiveTransaction();
        TransactionController::TransactionId generateTransactionId();
        void commitCurrentTransaction(const QString &name);
    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_P_H
