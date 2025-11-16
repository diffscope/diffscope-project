#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_H

#include <QObject>

class QAbstractItemModel;

namespace Core {

    class TransactionalStrategy;

    class TransactionControllerPrivate;

    class TransactionController : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(TransactionController)
        Q_PROPERTY(bool transactionActive READ isTransactionActive NOTIFY transactionActiveChanged)
        Q_PROPERTY(bool exclusiveToTransaction READ isExclusiveToTransaction NOTIFY exclusiveToTransactionChanged)
        Q_PROPERTY(int currentStep READ currentStep WRITE setCurrentStep NOTIFY currentStepChanged)
        Q_PROPERTY(int totalSteps READ totalSteps NOTIFY totalStepsChanged)
        Q_PROPERTY(int cleanStep READ cleanStep WRITE setCleanStep NOTIFY cleanStepChanged)
        Q_PROPERTY(QAbstractItemModel *stepModel READ stepModel CONSTANT)
    public:
        explicit TransactionController(TransactionalStrategy *strategy, QObject *parent = nullptr);
        ~TransactionController() override;

        TransactionalStrategy *strategy() const;

        bool isTransactionActive() const;
        bool isExclusiveToTransaction() const;

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

        Q_INVOKABLE TransactionId beginNonExclusiveTransaction(int lifeTime, const QString &name);
        Q_INVOKABLE void resetNonExclusiveTransactionLifeTime(TransactionId transactionId, int lifeTime, const QString &name = {});

    Q_SIGNALS:
        void transactionActiveChanged(bool isTransactionActive);
        void exclusiveToTransactionChanged(bool isExclusiveToTransaction);
        void currentStepChanged(int currentStep);
        void totalStepsChanged(int totalSteps);
        void cleanStepChanged(int cleanStep);

        void transactionBegun();
        void transactionAborted();
        void transactionCommitted(const QString &name);
        void nonExclusiveTransactionAutoCommitted(TransactionId transactionId);

    private:
        QScopedPointer<TransactionControllerPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTIONCONTROLLER_H
