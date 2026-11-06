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

    class DSPXPROJECT_EXPORT EditTempoTimeSignatureStrategy : public QObject {
        Q_OBJECT
    public:
        explicit EditTempoTimeSignatureStrategy(QObject *parent = nullptr);
        ~EditTempoTimeSignatureStrategy() override;

        struct TempoInput {
            int position;
            bool insert;
            double tempo;
        };

        struct TimeSignatureInput {
            int position;
            bool insert;
            int numerator;
            int denominator;
        };

        virtual std::optional<TempoInput> requestTempo(const TempoInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual std::optional<TimeSignatureInput> requestTimeSignature(const TimeSignatureInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESTRATEGY_H
