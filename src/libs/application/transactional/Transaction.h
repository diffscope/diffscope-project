#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTION_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTION_H

#include <QObject>

namespace Core {

    class TransactionController;

    class TransactionPrivate;

    class Transaction : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Transaction)
        Q_PROPERTY(Status status READ status NOTIFY statusChanged)
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    public:
        ~Transaction() override;

        TransactionController *controller() const;

        Q_INVOKABLE bool begin();
        Q_INVOKABLE void abort();
        Q_INVOKABLE bool commit();

        Q_INVOKABLE bool beginAutoCommittable();
        Q_INVOKABLE void extendAutoCommitTimeout(int timeoutMs);

        enum Status {
            Pending,
            Running,
            Committed,
            Aborted,
            AbortDeferred,
        };
        Q_ENUM(Status)
        Status status() const;

        QString name() const;
        void setName(const QString &name);

    Q_SIGNALS:
        void statusChanged(Status status);
        void nameChanged(const QString &name);

    private:
        friend class TransactionController;
        QScopedPointer<TransactionPrivate> d_ptr;

        explicit Transaction(TransactionController *controller);
    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTION_H
