// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_TRANSACTIONAL_TRANSACTIONALSTRATEGY_H
#define DIFFSCOPE_TRANSACTIONAL_TRANSACTIONALSTRATEGY_H

#include <QObject>

namespace Core {

    class TransactionalStrategy : public QObject {
        Q_OBJECT
    public:
        explicit TransactionalStrategy(QObject *parent = nullptr);
        ~TransactionalStrategy() override;

        virtual bool beginTransaction() = 0;
        virtual void abortTransaction() = 0;
        // Returns whether the commit created an undo step.
        virtual bool commitTransaction() = 0;
        virtual void moveCurrentStepBy(int count) = 0;

    };

}

#endif //DIFFSCOPE_TRANSACTIONAL_TRANSACTIONALSTRATEGY_H
