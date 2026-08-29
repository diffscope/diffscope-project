// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISPROJECTADDON_P_H
#define DIFFSCOPE_SYNTH_SYNTHESISPROJECTADDON_P_H

#include <synth/internal/SynthesisProjectAddOn.h>

#include <optional>

#include <QHash>
#include <QList>
#include <QPointer>
#include <QString>
#include <QStringList>

#include <audio/TrackAudioContext.h>
#include <dspxmodelORM/Handle.h>
#include <synth/ServiceTypes.h>
#include <synth/SynthesisModel.h>

// We have to include this to make lupdate know where `SynthesisProjectAddOn` is
// Otherwise it will warn that the class is not found
#ifdef QT_LUPDATE_ONLY
#   include "SynthesisProjectAddOn.h"
#endif

namespace dspx {
    class ClipWatcher;
    class Piece;
    class PieceDivider;
}

namespace talcs {
    class FutureAudioSourceClipSeries;
}

namespace Synth::Internal {

    struct SynthesisProjectAddOn::ClipRuntime {
        struct LanguageContinuation {
            double position{};
            double length{};
            SynthesisTaskOptions options;
            ArchitectureMetadata architecture;
            bool failed{};
            bool canceled{};
            QString errorMessage;
        };

        dspx::Handle clipHandle{};
        QPointer<dspx::SingingClip> clip;
        dspx::PieceDivider *divider{};
        dspx::ClipWatcher *watcher{};
        QHash<dspx::Piece *, SynthesisPiece *> pieces;
        QPointer<Audio::TrackAudioContext> audioTrackContext;
        talcs::FutureAudioSourceClipSeries *audioSeries{};
        QList<LanguageContinuation> languageContinuations;
        std::optional<SynthesisContext> synthesisContextSnapshot;
        bool rebound{};
    };

    struct SynthesisProjectAddOn::TaskWriteback {
        enum Scope {
            Language,
            Piece,
        };

        Scope scope{Piece};
        QPointer<SynthesisTask> task;
        dspx::Handle clipHandle{};
        QPointer<SynthesisPiece> piece;
        SynthesisTaskType type{SynthesisTaskType::Pronunciation};
        SynthesisTaskOptions options;
        ArchitectureMetadata architecture;
        SynthesisTaskRequest request;
        QList<dspx::Handle> noteHandles;
        std::optional<QStringList> requestedParameters;
        quint64 revision{};
        double piecePosition{};
        double pieceLength{};
        QString responseShapeError;
        bool queued{};
        bool discarded{};
    };

    struct SynthesisProjectAddOn::ManualRequest {
        enum Scope {
            Project,
            Clip,
            Piece,
        };

        Scope scope{Project};
        dspx::Handle clipHandle{};
        double position{};
        double length{};
        SynthesisTaskType fromType{SynthesisTaskType::Pronunciation};
        SynthesisTaskOptions options;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPROJECTADDON_P_H
