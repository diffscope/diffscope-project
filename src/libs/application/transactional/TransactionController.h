#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_H

#include <QObject>

namespace Core {

    class TransactionalStrategy;
    class Transaction;

    class TransactionControllerPrivate;

    class TransactionController : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(TransactionController)
        Q_PROPERTY(bool inTransaction READ isInTransaction NOTIFY inTransactionChanged)
        Q_PROPERTY(int currentStep READ currentStep WRITE setCurrentStep NOTIFY currentStepChanged)
        Q_PROPERTY(int totalSteps READ totalSteps NOTIFY totalStepsChanged)
        Q_PROPERTY(int cleanStep READ cleanStep WRITE setCleanStep NOTIFY cleanStepChanged)
    public:
        explicit TransactionController(TransactionalStrategy *strategy, QObject *parent = nullptr);
        ~TransactionController() override;

        TransactionalStrategy *strategy() const;

        Q_INVOKABLE Transaction *createTransaction();

        bool isInTransaction() const;

        int currentStep() const;
        void setCurrentStep(int step);

        int totalSteps() const;

        int cleanStep() const;
        void setCleanStep(int step);

        Q_INVOKABLE QString stepName(int step) const;

    Q_SIGNALS:
        void inTransactionChanged(bool isInTransaction);
        void currentStepChanged(int currentStep);
        void totalStepsChanged(int totalSteps);
        void cleanStepChanged(int cleanStep);

    private:
        QScopedPointer<TransactionControllerPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_H
