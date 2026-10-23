// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_TEMPOEXTRACTIONPLANNER_H
#define DIFFSCOPE_SYNTH_TEMPOEXTRACTIONPLANNER_H

#include <atomic>
#include <memory>

#include <QMap>
#include <QPointF>
#include <QString>
#include <QVector>

#include <SVSCraftCore/MusicTimeSignature.h>

#include <synth/internal/AudioExtractionTask.h>

namespace Synth::Internal {

    struct TempoExtractionInput {
        QList<ExtractionBeat> beats;
        QMap<int, double> tempi;
        QMap<int, SVS::MusicTimeSignature> timeSignatures;
        int resolution{};
        int position{};
        int clipStart{};
        int clipLength{};
        int length{};
        double startSeconds{};
        double durationSeconds{};
    };

    struct TempoExtractionOptions {
        bool variable{};
        bool automatic{true};
        bool automaticReference{true};
        bool moveAudio{true};
        double bpm{120};
        double smoothing{75};
        int minimumSegmentBeats{4};
        int referenceBeat{};
        int referenceTick{};
    };

    struct TempoExtractionPlan {
        QString error;
        QString keepAudioPositionError;
        QMap<int, double> tempi;
        QVector<QPointF> candidateBeats;
        QVector<QPointF> tempoPoints;
        QVector<QPointF> errors;
        QVector<int> beatTicks;
        int referenceTick{};
        int position{};
        int begin{};
        int end{};
        int clipStart{};
        int clipLength{};
        int length{};
        int fittedCount{};
        bool moveAudio{true};
        double bpm{};
        double maxErrorMsec{};
        double audioShiftMsec{};
    };

    TempoExtractionPlan planExtractedTempo(const TempoExtractionInput &input, const TempoExtractionOptions &options, const std::shared_ptr<std::atomic_bool> &cancelled);

}

#endif // DIFFSCOPE_SYNTH_TEMPOEXTRACTIONPLANNER_H
