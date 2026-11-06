// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITLOOPSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_EDITLOOPSTRATEGY_H

#include <optional>

#include <QObject>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace SVS {
    class MusicTimeline;
}

namespace DspxProject {

    class DSPXPROJECT_EXPORT EditLoopStrategy : public QObject {
        Q_OBJECT
    public:
        explicit EditLoopStrategy(QObject *parent = nullptr);
        ~EditLoopStrategy() override;

        struct Input {
            bool enabled;
            int start;
            int end;
        };

        virtual std::optional<Input> requestLoop(const Input &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITLOOPSTRATEGY_H
