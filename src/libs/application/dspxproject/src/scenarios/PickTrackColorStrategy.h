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

    class DSPXPROJECT_EXPORT PickTrackColorStrategy : public QObject {
        Q_OBJECT
    public:
        explicit PickTrackColorStrategy(QObject *parent = nullptr);
        ~PickTrackColorStrategy() override;

        virtual std::optional<int> requestColor(int currentColorId, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSTRATEGY_H
