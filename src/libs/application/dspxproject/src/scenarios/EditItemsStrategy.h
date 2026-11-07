// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITITEMSSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_EDITITEMSSTRATEGY_H

#include <QObject>

#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    /** Supplies bounce confirmations. Window arguments are optional dialog parents. */
    class DSPXPROJECT_EXPORT EditItemsStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit EditItemsStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~EditItemsStrategy() override;

        /** Returns whether the user accepts differing singer layouts during bouncing. */
        virtual bool confirmSingerLayoutChange(QQuickWindow *window) = 0;
        /** Returns whether the user accepts data loss during bouncing. */
        virtual bool confirmBounceDataLoss(QQuickWindow *window) = 0;
        /** Reports a conflicting operation that prevents a transaction. */
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITITEMSSTRATEGY_H
