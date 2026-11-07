// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSTRATEGY_H

#include <optional>

#include <QObject>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    /** Supplies track color interaction. Window arguments are optional dialog parents. */
    class DSPXPROJECT_EXPORT PickTrackColorStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit PickTrackColorStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~PickTrackColorStrategy() override;

        /** Returns the accepted color identifier, or nullopt on cancellation. */
        virtual std::optional<int> requestColor(int currentColorId, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Reports a conflicting operation that prevents a transaction. */
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSTRATEGY_H
