// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESTRATEGY_H

#include <optional>

#include <QObject>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace SVS {
    class MusicTimeline;
}

namespace DspxProject {

    /** Supplies tempo and meter interaction with optional window parents and required timelines. */
    class DSPXPROJECT_EXPORT EditTempoTimeSignatureStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit EditTempoTimeSignatureStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~EditTempoTimeSignatureStrategy() override;

        /** The initial or accepted tempo values. */
        struct TempoInput {
            /** The project tick position. */
            int position;
            /** Whether to insert instead of modifying the effective tempo. */
            bool insert;
            /** The tempo in beats per minute. Accepted values range from 10 through 1000. */
            double tempo;
        };

        /** The initial or accepted meter values. */
        struct TimeSignatureInput {
            /** The project tick position within the target measure. */
            int position;
            /** Whether to insert instead of modifying the effective meter. */
            bool insert;
            /** The positive meter numerator. */
            int numerator;
            /** The meter denominator, a positive power of two no greater than 128. */
            int denominator;
        };

        /** Returns accepted tempo values, or nullopt on cancellation. @pre timeline is non-null. */
        virtual std::optional<TempoInput> requestTempo(const TempoInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Returns accepted meter values, or nullopt on cancellation. @pre timeline is non-null. */
        virtual std::optional<TimeSignatureInput> requestTimeSignature(const TimeSignatureInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Reports a conflicting operation that prevents a transaction. */
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESTRATEGY_H
