#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_P_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_P_H

#include <transactional/TransactionController.h>

namespace Core {

    class TransactionControllerPrivate {
        Q_DECLARE_PUBLIC(TransactionController)
    public:
        TransactionController *q_ptr;

        TransactionalStrategy *strategy;
    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_P_H
