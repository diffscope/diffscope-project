// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITITEMSSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_EDITITEMSSTRATEGY_H

#include <QObject>

#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class DSPXPROJECT_EXPORT EditItemsStrategy : public QObject {
        Q_OBJECT
    public:
        explicit EditItemsStrategy(QObject *parent = nullptr);
        ~EditItemsStrategy() override;

        virtual bool confirmSingerLayoutChange(QQuickWindow *window) = 0;
        virtual bool confirmBounceDataLoss(QQuickWindow *window) = 0;
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITITEMSSTRATEGY_H
