#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTIONALSTRATEGY_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTIONALSTRATEGY_H

#include <QObject>

namespace Core {

    class TransactionalStrategy : public QObject {
        Q_OBJECT
    public:
        explicit TransactionalStrategy(QObject *parent = nullptr);
        ~TransactionalStrategy() override;

        virtual void beginTransaction() = 0;
        virtual void abortTransaction() = 0;
        virtual void commitTransaction() = 0;
        virtual void moveCurrentStep(int step) = 0;

    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTIONALSTRATEGY_H
