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

    /** Supplies loop interaction with optional window parents and required timelines. */
    class DSPXPROJECT_EXPORT EditLoopStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit EditLoopStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~EditLoopStrategy() override;

        /** The initial or accepted loop state and half-open range. */
        struct Input {
            /** Whether looping is enabled. */
            bool enabled;
            /** The inclusive start in project ticks. */
            int start;
            /** The exclusive end in project ticks. */
            int end;
        };

        /**
         * Returns accepted values, or nullopt on cancellation.
         * @pre timeline is non-null and valid during the interaction.
         */
        virtual std::optional<Input> requestLoop(const Input &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Reports a conflicting operation that prevents a transaction. */
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITLOOPSTRATEGY_H
