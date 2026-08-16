// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISPIECE_P_H
#define DIFFSCOPE_SYNTH_SYNTHESISPIECE_P_H

#include <QPointer>

#include <synth/SynthesisPiece.h>

namespace Synth {

    class SynthesisPiecePrivate {
        Q_DECLARE_PUBLIC(SynthesisPiece)

    public:
        explicit SynthesisPiecePrivate(SynthesisPiece *q) : q_ptr(q) {}
        static SynthesisPiecePrivate *get(SynthesisPiece *piece) { return piece->d_func(); }

        SynthesisPiece *q_ptr{};
        QUuid id{QUuid::createUuid()};
        QPointer<dspx::SingingClip> clip;
        double position{};
        double length{};
        SynthesisPiece::State state{SynthesisPiece::Idle};
        SynthesisTaskType currentTaskType{SynthesisTaskType::Pronunciation};
        QString audioFilePath;
        QString errorMessage;
        quint64 revision{};
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPIECE_P_H
