// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISPROJECTINPUT_H
#define DIFFSCOPE_SYNTH_SYNTHESISPROJECTINPUT_H

#include <optional>

#include <QList>
#include <QStringList>

#include <dspxmodelORM/Handle.h>
#include <synth/ParameterConfiguration.h>
#include <synth/ServiceTypes.h>
#include <synth/SynthesisModel.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace SVS {
    class MusicTimeline;
}

namespace dspx {
    class PhonemeSequence;
    class PieceDivider;
    class SingingClip;
}

namespace Synth {
    class SynthesisPiece;
}

namespace Synth::Internal::ProjectInput {

    struct BuiltScore {
        SynthesisScore score;
        QList<dspx::Handle> noteHandles;
        QString error;
    };

    struct BuiltLanguageRequest {
        SynthesisTaskRequest request;
        QList<dspx::Handle> noteHandles;
    };

    void configureDivider(dspx::PieceDivider *divider);
    bool isManagedClip(dspx::SingingClip *clip);
    bool rangesOverlap(double leftPosition, double leftLength, double rightPosition, double rightLength);
    bool sameSynthesisInput(SynthesisTaskRequest left, SynthesisTaskRequest right);

    ArchitectureMetadata architectureFor(const SynthesisContext &context);
    QStringList downstreamIndirectParameters(const ArchitectureMetadata &architecture, const QStringList &changedParameters);
    std::optional<SynthesisContext> buildSynthesisContext(dspx::SingingClip *clip);
    ParameterConfiguration parameterConfiguration(const QString &architectureId, const QString &parameterId);

    SynthesisTaskType executableStage(const ArchitectureMetadata &architecture, SynthesisTaskType requestedStage);
    SynthesisTaskType nextStage(const ArchitectureMetadata &architecture, SynthesisTaskType completedStage);

    BuiltScore buildScore(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, const ArchitectureMetadata &architecture, bool forAudio, const std::optional<QStringList> &requestedParameters = std::nullopt);
    BuiltLanguageRequest buildLanguageRequest(dspx::SingingClip *clip, double piecePosition, double pieceLength, SynthesisTaskType type, const SynthesisContext &context);

    double tickSeconds(SVS::MusicTimeline *timeline, double tick);

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPROJECTINPUT_H
