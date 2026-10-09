// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisProjectInput.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>
#include <vector>

#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelPiece/PieceDivider.h>
#include <synth/SynthInterface.h>
#include <synth/SynthesisPiece.h>
#include <synth/internal/SynthService.h>
#include <synth/internal/SynthesisParameterEvaluator.h>
#include <synth/internal/SynthesisProjectAddOn.h>

namespace Synth::Internal::ProjectInput {

    namespace {

        struct FlattenedSinger {
            SynthesisSinger singer;
            int rootIndex{};
            double nestedWeight{};
        };

        struct RootMixAnchor {
            int position{};
            std::vector<double> weights;
        };

        struct RootMixCurve {
            int rootCount{};
            std::vector<RootMixAnchor> anchors;
        };

        double configuredSampleRate(const QString &key, double fallback) {
            auto settings = Core::RuntimeInterface::settings();
            settings->beginGroup(QStringLiteral("org.diffscope.synth"));
            const double value = settings->value(key, fallback).toDouble();
            settings->endGroup();
            return std::max(1.0, value);
        }

        std::vector<double> logicalWeights(const QList<double> &stored, int count) {
            if (count <= 0) {
                return {};
            }
            std::vector<double> result(static_cast<std::size_t>(count), 0.0);
            double sum{};
            for (int index = 0; index < count - 1; ++index) {
                const double value = index < stored.size() ? std::clamp(stored.at(index), 0.0, 1.0) : 0.0;
                result[static_cast<std::size_t>(index)] = value;
                sum += value;
            }
            result.back() = std::max(0.0, 1.0 - sum);
            return result;
        }

        void flattenSinger(dspx::Singer *singer, int rootIndex, double weight, QList<FlattenedSinger> &result) {
            if (!singer) {
                return;
            }
            if (singer->kind() == dspx::Singer::Single) {
                auto single = static_cast<dspx::SingleSinger *>(singer);
                result.append({{single->id(), singer->extra()}, rootIndex, weight});
                return;
            }
            auto mixed = static_cast<dspx::MixedSinger *>(singer);
            const auto children = mixed->singers()->items();
            const auto weights = logicalWeights(mixed->ratio(), children.size());
            for (int index = 0; index < children.size(); ++index) {
                flattenSinger(children.at(index), rootIndex, weight * weights[static_cast<std::size_t>(index)], result);
            }
        }

        QJsonValue architectureExtra(const QString &architectureId) {
            auto settings = Core::RuntimeInterface::settings();
            settings->beginGroup(QStringLiteral("org.diffscope.synth"));
            const auto encoded = settings->value(QStringLiteral("architectureExtras")).toByteArray();
            settings->endGroup();
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(encoded, &error);
            if (error.error != QJsonParseError::NoError || !document.isObject()) {
                return QJsonValue::Undefined;
            }
            const auto value = document.object().value(architectureId);
            return value;
        }

        std::optional<SynthesisContext> buildContext(dspx::SingingClip *clip, QList<FlattenedSinger> *flattened) {
            const auto sources = clip ? clip->sources() : nullptr;
            if (!sources || sources->category().isEmpty()) {
                return std::nullopt;
            }
            QList<FlattenedSinger> leaves;
            const auto roots = sources->singers()->items();
            for (int index = 0; index < roots.size(); ++index) {
                flattenSinger(roots.at(index), index, 1.0, leaves);
            }
            if (leaves.isEmpty()) {
                return std::nullopt;
            }
            SynthesisContext result;
            result.architectureId = sources->category();
            result.architectureExtra = architectureExtra(result.architectureId);
            for (const auto &leaf : leaves) {
                result.singers.append(leaf.singer);
            }
            const auto pipeline = pipelineFor(result);
            if (!pipeline.architecture.id().isEmpty()) {
                if (result.architectureExtra.isUndefined())
                    result.architectureExtra = pipeline.group.defaultArchExtra;
            }
            if (flattened) {
                *flattened = leaves;
            }
            return result;
        }

        QString effectivePronunciation(dspx::Note *note) {
            if (!note->editedPronunciation().isEmpty()) {
                return note->editedPronunciation();
            }
            if (!note->originalPronunciation().isEmpty()) {
                return note->originalPronunciation();
            }
            return note->lyric();
        }

        struct NoteText {
            QString lyric;
            QString pronunciation;
            QString language;
            bool slur{};
            bool continuation{};
            bool emptySyllableSlice{};
            int syllableOffset{};
        };

        QList<NoteText> noteTexts(const QList<dspx::Note *> &notes, const QString &defaultLanguage) {
            QList<NoteText> result;
            for (auto note : notes) {
                NoteText text;
                text.lyric = note->lyric();
                text.pronunciation = effectivePronunciation(note);
                text.language = note->language().isEmpty() ? defaultLanguage : note->language();
                text.slur = note->lyric() == QStringLiteral("-");
                text.continuation = text.slur || note->lyric() == QStringLiteral("+");
                text.emptySyllableSlice = text.slur;
                if (text.continuation) {
                    if (result.isEmpty()) {
                        text.lyric.clear();
                        text.pronunciation.clear();
                        text.emptySyllableSlice = true;
                    } else {
                        const auto &previous = result.last();
                        text.lyric = previous.lyric;
                        text.pronunciation = previous.pronunciation;
                        text.language = previous.language;
                        text.emptySyllableSlice = text.slur || previous.emptySyllableSlice;
                        text.syllableOffset = previous.syllableOffset + (text.slur ? 0 : 1);
                    }
                }
                result.append(text);
            }
            return result;
        }

        dspx::PhonemeSequence *effectivePhonemes(dspx::Note *note) {
            return note->editedPhonemes()->size() > 0 ? note->editedPhonemes() : note->originalPhonemes();
        }

        RootMixCurve buildRootMixCurve(dspx::Sources *sources, int minimumTick, int maximumTick) {
            RootMixCurve result;
            if (!sources) {
                return result;
            }
            result.rootCount = sources->singers()->size();
            if (maximumTick < minimumTick) {
                return result;
            }
            const int slicePosition = std::max(0, minimumTick);
            const qint64 sliceEnd = std::max(static_cast<qint64>(slicePosition) + 1,
                                             static_cast<qint64>(maximumTick) + 1);
            const int sliceLength = static_cast<int>(std::min(
                sliceEnd - slicePosition,
                static_cast<qint64>(std::numeric_limits<int>::max())));
            const auto anchors = sources->dynamicMixingAnchors()->sliceEffective(slicePosition,
                                                                                 sliceLength);
            result.anchors.reserve(static_cast<std::size_t>(anchors.size()));
            for (const auto anchor : anchors) {
                result.anchors.push_back({
                    anchor->position(),
                    logicalWeights(anchor->ratio(), result.rootCount),
                });
            }
            return result;
        }

        std::vector<double> rootWeightsAt(const RootMixCurve &curve, double position) {
            if (curve.anchors.empty()) {
                return logicalWeights({}, curve.rootCount);
            }
            const auto right = std::lower_bound(
                curve.anchors.cbegin(), curve.anchors.cend(), position,
                [](const RootMixAnchor &anchor, double value) {
                    return anchor.position < value;
                }
            );
            if (right == curve.anchors.cbegin()) {
                return right->weights;
            }
            if (right == curve.anchors.cend()) {
                return curve.anchors.back().weights;
            }
            const auto left = right - 1;
            const double ratio = std::clamp((position - left->position) /
                                                (right->position - left->position),
                                            0.0, 1.0);
            auto result = left->weights;
            for (std::size_t index = 0; index < result.size(); ++index) {
                result[index] += (right->weights[index] - result[index]) * ratio;
            }
            return result;
        }

        int globalCentShift(dspx::SingingClip *clip) {
            return clip && clip->model() ? clip->model()->globalCentShift() : 0;
        }

        int pitchAt(dspx::SingingClip *clip, int relativeTick) {
            for (auto note : clip->notes()->asRange()) {
                if (relativeTick >= note->position() && relativeTick < note->position() + note->length()) {
                    return note->keyNumber() * 100 + note->centShift();
                }
            }
            return 0;
        }

    }

    void configureDivider(dspx::PieceDivider *divider) {
        if (!divider) {
            return;
        }
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synth"));
        divider->setPaddingBase(settings->value(QStringLiteral("piecePaddingBaseMs"), 100.0).toDouble());
        divider->setPaddingAdditional(settings->value(QStringLiteral("piecePaddingAdditionalMs"), 100.0).toDouble());
        divider->setPaddingGap(settings->value(QStringLiteral("piecePaddingGapMs"), 200.0).toDouble());
        divider->setRestLyrics(settings->value(
                                           QStringLiteral("pieceRestLyrics"),
                                           QStringList{QStringLiteral("AP"), QStringLiteral("SP")}
        )
                                   .toStringList());
        settings->endGroup();
    }

    bool isManagedClip(dspx::SingingClip *clip) {
        const auto service = SynthService::instance();
        const auto sources = clip ? clip->sources() : nullptr;
        return service && sources && service->managesArchitecture(sources->category());
    }

    bool rangesOverlap(double leftPosition, double leftLength, double rightPosition, double rightLength) {
        return leftPosition < rightPosition + rightLength && rightPosition < leftPosition + leftLength;
    }

    bool sameSynthesisInput(SynthesisTaskRequest left, SynthesisTaskRequest right) {
        left.displayName.clear();
        right.displayName.clear();
        if (left.type == SynthesisTaskType::Duration) {
            left.score.parameters.clear();
            left.score.requestedParameters.clear();
            right.score.parameters.clear();
            right.score.requestedParameters.clear();
        }
        return left == right;
    }

    SynthesisPipeline pipelineFor(const SynthesisContext &context) {
        const auto interface = SynthInterface::instance();
        if (!interface)
            return {};
        for (const bool healthyOnly : {true, false}) {
            for (const auto &service : interface->serviceInstances()) {
                const auto details = interface->serviceInstanceDetails(service.id());
                if (!service.isEnabled() || details.healthStatus() == ServiceInstanceDetails::Disabled
                    || (details.healthStatus() == ServiceInstanceDetails::Healthy) != healthyOnly)
                    continue;
                const auto pipeline = SynthesisMetadataAccess::resolve(details.metadata(), context);
                if (pipeline)
                    return *pipeline;
            }
        }
        return {};
    }

    QStringList downstreamIndirectParameters(const SynthesisPipeline &pipeline, const QStringList &changedParameters) {
        const QSet<QString> editedParameters(changedParameters.cbegin(), changedParameters.cend());
        QSet<QString> affectedParameters = editedParameters;
        QSet<QString> requestedParameters;
        bool progressed = true;
        while (progressed) {
            progressed = false;
            for (auto it = pipeline.group.parameterPipeline.cbegin(); it != pipeline.group.parameterPipeline.cend(); ++it) {
                const auto &metadata = it.value();
                if (metadata.type != Api::V1::ParameterPipelineMetadata::Indirect || editedParameters.contains(it.key()) || requestedParameters.contains(it.key())) {
                    continue;
                }
                const bool dependsOnAffectedParameter = std::ranges::any_of(metadata.dependsOn, [&affectedParameters](const QString &dependency) {
                    return affectedParameters.contains(dependency);
                });
                if (!dependsOnAffectedParameter) {
                    continue;
                }
                requestedParameters.insert(it.key());
                affectedParameters.insert(it.key());
                progressed = true;
            }
        }

        QStringList result;
        for (auto it = pipeline.group.parameterPipeline.cbegin(); it != pipeline.group.parameterPipeline.cend(); ++it) {
            if (requestedParameters.contains(it.key())) {
                result.append(it.key());
            }
        }
        return result;
    }

    std::optional<SynthesisContext> buildSynthesisContext(dspx::SingingClip *clip) {
        return buildContext(clip, nullptr);
    }

    SynthesisTaskType executableStage(const SynthesisPipeline &pipeline, SynthesisTaskType requestedStage) {
        if (requestedStage == SynthesisTaskType::Pronunciation
            && std::ranges::none_of(pipeline.group.languages, [](const auto &language) { return language.pronunciationMode == QStringLiteral("full"); }))
            requestedStage = SynthesisTaskType::Phoneme;
        if (requestedStage == SynthesisTaskType::Phoneme
            && std::ranges::none_of(pipeline.group.languages, [](const auto &language) { return language.phonemeMode != QStringLiteral("skip"); }))
            requestedStage = SynthesisTaskType::Duration;
        if (requestedStage == SynthesisTaskType::Duration && pipeline.group.durationMode == QStringLiteral("skip"))
            requestedStage = SynthesisTaskType::Parameter;
        return requestedStage;
    }

    SynthesisTaskType nextStage(const SynthesisPipeline &pipeline, SynthesisTaskType completedStage) {
        switch (completedStage) {
            case SynthesisTaskType::Pronunciation: return executableStage(pipeline, SynthesisTaskType::Phoneme);
            case SynthesisTaskType::Phoneme: return executableStage(pipeline, SynthesisTaskType::Duration);
            case SynthesisTaskType::Duration: return SynthesisTaskType::Parameter;
            case SynthesisTaskType::Parameter:
            case SynthesisTaskType::Audio: return SynthesisTaskType::Audio;
        }
        return SynthesisTaskType::Audio;
    }

    BuiltScore buildScore(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, const SynthesisPipeline &pipeline, bool forAudio, const std::optional<QStringList> &requestedParameters) {
        BuiltScore result;
        if (!window || !clip || !piece || !clip->sources()) {
            result.error = Synth::Internal::SynthesisProjectAddOn::tr("The synthesis piece is no longer attached to a valid clip");
            return result;
        }
        auto timeline = window->projectTimeline()->musicTimeline();
        const double pieceStartTick = clip->start() + piece->position();
        const double pieceEndTick = pieceStartTick + piece->length();
        const double pieceStartSeconds = tickSeconds(timeline, pieceStartTick);
        const double pieceEndSeconds = tickSeconds(timeline, pieceEndTick);
        result.score.pieceDuration = std::max(0.0, pieceEndSeconds - pieceStartSeconds);
        result.score.mixSampleRate = configuredSampleRate(QStringLiteral("mixSampleRate"), 100.0);

        QList<dspx::Note *> allNotes;
        for (auto note : clip->notes()->asRange())
            allNotes.append(note);
        std::sort(allNotes.begin(), allNotes.end(), [](auto left, auto right) { return left->position() < right->position(); });
        const auto allTexts = noteTexts(allNotes, pipeline.defaultLanguage);
        QList<dspx::Note *> notes;
        QList<NoteText> texts;
        for (qsizetype index = 0; index < allNotes.size(); ++index) {
            const double noteTick = clip->start() + allNotes.at(index)->position();
            if (noteTick >= pieceStartTick && noteTick < pieceEndTick) {
                notes.append(allNotes.at(index));
                texts.append(allTexts.at(index));
            }
        }
        qsizetype noteIndex{};
        double previousEnd = pieceStartSeconds;
        const int documentCentShift = globalCentShift(clip);
        for (auto note : notes) {
            const double noteStart = tickSeconds(timeline, clip->start() + note->position());
            const double noteEnd = tickSeconds(timeline, clip->start() + note->position() + note->length());
            if (noteStart + 1e-9 < previousEnd) {
                result.error = Synth::Internal::SynthesisProjectAddOn::tr("Some notes overlap. Move or resize the overlapping notes before synthesizing");
                return result;
            }
            SynthesisScoreNote converted;
            converted.gap = std::max(0.0, noteStart - previousEnd);
            converted.duration = std::max(0.0, noteEnd - noteStart);
            converted.cent = std::clamp(note->keyNumber() * 100 + note->centShift() + documentCentShift, 0, 12800);
            const auto &text = texts.at(noteIndex++);
            converted.pronunciation = text.pronunciation;
            converted.language = text.language;
            converted.slur = text.slur;
            if (converted.slur && (result.score.notes.isEmpty() || converted.gap > 1e-9)) {
                result.error = Synth::Internal::SynthesisProjectAddOn::tr("A slur note must immediately follow another note");
                return result;
            }
            if (converted.slur)
                converted.gap = 0.0;
            for (auto phoneme : effectivePhonemes(note)->asRange()) {
                if (converted.slur)
                    break;
                converted.phonemes.append({
                    phoneme->token(),
                    phoneme->onset(),
                    phoneme->language().isEmpty() ? text.language : phoneme->language(),
                    phoneme->start() / 1000.0,
                });
            }
            result.score.notes.append(converted);
            result.noteHandles.append(note->handle());
            previousEnd = noteEnd;
        }

        QList<FlattenedSinger> leaves;
        buildContext(clip, &leaves);
        const int mixFrames = std::max(1, static_cast<int>(std::ceil(result.score.pieceDuration * result.score.mixSampleRate)));
        std::vector<double> mixTicks;
        mixTicks.reserve(static_cast<std::size_t>(mixFrames));
        for (int frame = 0; frame < mixFrames; ++frame) {
            const double seconds = pieceStartSeconds + frame / result.score.mixSampleRate;
            mixTicks.push_back(timeline->create(seconds * 1000.0).totalTick() - clip->start());
        }
        const auto [minimumMixTick, maximumMixTick] = std::minmax_element(mixTicks.cbegin(), mixTicks.cend());
        const auto rootMixCurve = buildRootMixCurve(clip->sources(),
                                                    static_cast<int>(std::floor(*minimumMixTick)),
                                                    static_cast<int>(std::floor(*maximumMixTick)));
        for (const double tick : mixTicks) {
            const auto rootWeights = rootWeightsAt(rootMixCurve, tick);
            QList<double> weights;
            double sum{};
            for (const auto &leaf : leaves) {
                const double value = leaf.rootIndex < static_cast<int>(rootWeights.size())
                                         ? rootWeights[static_cast<std::size_t>(leaf.rootIndex)] * leaf.nestedWeight
                                         : 0.0;
                weights.append(value);
                sum += value;
            }
            if (sum > 0.0) {
                for (double &value : weights) {
                    value /= sum;
                }
            }
            if (!weights.isEmpty()) {
                weights.removeLast();
            }
            result.score.mix.append(weights);
        }

        const double parameterSampleRate = configuredSampleRate(QStringLiteral("parameterSampleRate"), 100.0);
        const int parameterFrames = std::max(1, static_cast<int>(std::ceil(result.score.pieceDuration * parameterSampleRate)));
        QStringList parameterIds;
        if (forAudio) {
            parameterIds = pipeline.group.audioDependencies;
        } else {
            if (requestedParameters) {
                result.score.requestedParameters = *requestedParameters;
            } else {
                for (auto it = pipeline.group.parameterPipeline.cbegin(); it != pipeline.group.parameterPipeline.cend(); ++it) {
                    const auto &metadata = it.value();
                    if (metadata.type == Api::V1::ParameterPipelineMetadata::Indirect) {
                        result.score.requestedParameters.append(it.key());
                    }
                }
            }
            result.score.requestedParameters.removeDuplicates();
            for (auto it = pipeline.group.parameterPipeline.cbegin(); it != pipeline.group.parameterPipeline.cend(); ++it) {
                parameterIds.append(it.key());
            }
        }
        parameterIds.removeDuplicates();
        if (parameterIds.isEmpty()) {
            return result;
        }
        QList<int> parameterTicks;
        parameterTicks.reserve(parameterFrames);
        for (int frame = 0; frame < parameterFrames; ++frame) {
            const double seconds = pieceStartSeconds + frame / parameterSampleRate;
            parameterTicks.append(timeline->create(seconds * 1000.0).totalTick() - clip->start());
        }
        const auto [minimumTick, maximumTick] = std::minmax_element(parameterTicks.cbegin(), parameterTicks.cend());
        for (const auto &id : parameterIds) {
            SynthesisParameter parameter;
            parameter.sampleRate = parameterSampleRate;
            const double fallback = pipeline.architecture.parameters().value(id).toObject().value(QStringLiteral("defaultValue")).toDouble();
            const SynthesisParameterEvaluator evaluator(clip->parameters()->item(id), *minimumTick, *maximumTick);
            for (const int relativeTick : parameterTicks) {
                const double defaultValue = id == QStringLiteral("pitch")
                    ? std::clamp(static_cast<double>(pitchAt(clip, relativeTick)) / 12800.0, 0.0, 1.0)
                    : fallback;
                double value = evaluator.evaluate(relativeTick, defaultValue);
                if (id == QStringLiteral("pitch")) {
                    value = std::clamp(value + static_cast<double>(documentCentShift) / 12800.0,
                                       0.0, 1.0);
                }
                parameter.values.append(value);
            }
            result.score.parameters.insert(id, parameter);
        }
        return result;
    }

    BuiltLanguageRequest buildLanguageRequest(dspx::SingingClip *clip, double piecePosition, double pieceLength, SynthesisTaskType type, const SynthesisContext &context) {
        BuiltLanguageRequest result;
        result.request.type = type;
        result.request.context = context;
        result.request.displayName = clip->name();
        const auto pipeline = pipelineFor(context);
        QList<dspx::Note *> notes;
        for (auto note : clip->notes()->asRange())
            notes.append(note);
        std::sort(notes.begin(), notes.end(), [](auto left, auto right) { return left->position() < right->position(); });
        const auto texts = noteTexts(notes, pipeline.defaultLanguage);
        const double pieceEnd = piecePosition + pieceLength;
        for (qsizetype index = 0; index < notes.size(); ++index) {
            const auto note = notes.at(index);
            if (note->position() < piecePosition || note->position() >= pieceEnd)
                continue;
            const auto &text = texts.at(index);
            if (type == SynthesisTaskType::Pronunciation && text.continuation)
                continue;
            const auto language = pipeline.group.languages.constFind(text.language);
            if (language != pipeline.group.languages.cend()
                && ((type == SynthesisTaskType::Pronunciation && language->pronunciationMode == QStringLiteral("skip"))
                    || (type == SynthesisTaskType::Phoneme && language->phonemeMode == QStringLiteral("skip"))))
                continue;
            result.noteHandles.append(note->handle());
            if (type == SynthesisTaskType::Pronunciation) {
                result.request.lyricNotes.append({text.lyric, text.language});
            } else {
                SynthesisPronunciationNote converted{text.pronunciation, text.language};
                if (text.emptySyllableSlice || text.pronunciation.isEmpty()) {
                    converted.syllableSliceStart = 0;
                    converted.syllableSliceEnd = 0;
                } else {
                    converted.syllableSliceStart = text.syllableOffset;
                    if (index + 1 < texts.size() && notes.at(index + 1)->lyric() == QStringLiteral("+"))
                        converted.syllableSliceEnd = text.syllableOffset + 1;
                }
                result.request.pronunciationNotes.append(converted);
            }
        }
        return result;
    }

    double tickSeconds(SVS::MusicTimeline *timeline, double tick) {
        return timeline->create(0, 0, static_cast<int>(std::round(tick))).millisecond() / 1000.0;
    }

}
