#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTION_P_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTION_P_H

#include <transactional/Transaction.h>

namespace Core {

    class TransactionPrivate {
        Q_DECLARE_PUBLIC(Transaction)
    public:
        Transaction *q_ptr;
    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTION_P_H
