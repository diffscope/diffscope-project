#include "SynthesisProjectInput.h"

#include <algorithm>
#include <cmath>
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
            if (singer->type() == dspx::Singer::Single) {
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
                return QJsonValue::Null;
            }
            const auto value = document.object().value(architectureId);
            return value.isUndefined() ? QJsonValue(QJsonValue::Null) : value;
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

        dspx::PhonemeSequence *effectivePhonemes(dspx::Note *note) {
            return note->editedPhonemes()->size() > 0 ? note->editedPhonemes() : note->originalPhonemes();
        }

        std::vector<double> rootWeightsAt(dspx::Sources *sources, double position) {
            const int rootCount = sources->singers()->size();
            const auto anchors = sources->dynamicMixingAnchors();
            if (anchors->size() == 0) {
                return logicalWeights({}, rootCount);
            }
            auto items = anchors->asRange();
            dspx::DynamicMixingAnchor *left = nullptr;
            dspx::DynamicMixingAnchor *right = nullptr;
            for (auto anchor : items) {
                if (anchor->position() <= position) {
                    left = anchor;
                }
                if (anchor->position() >= position) {
                    right = anchor;
                    break;
                }
            }
            if (!left) {
                left = anchors->firstItem();
            }
            if (!right) {
                right = anchors->lastItem();
            }
            const auto leftWeights = logicalWeights(left->ratio(), rootCount);
            if (left == right || right->position() == left->position()) {
                return leftWeights;
            }
            const auto rightWeights = logicalWeights(right->ratio(), rootCount);
            const double ratio = std::clamp((position - left->position()) / (right->position() - left->position()), 0.0, 1.0);
            auto result = leftWeights;
            for (std::size_t index = 0; index < result.size(); ++index) {
                result[index] += (rightWeights[index] - result[index]) * ratio;
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

    ArchitectureMetadata architectureFor(const SynthesisContext &context) {
        auto interface = SynthInterface::instance();
        if (!interface) {
            return {};
        }
        const auto findArchitecture = [&context, interface](bool healthyOnly, ArchitectureMetadata *fallback) {
            for (const auto &service : interface->serviceInstances()) {
                const auto details = interface->serviceInstanceDetails(service.id());
                if (!service.isEnabled() ||
                    details.healthStatus() == ServiceInstanceDetails::Disabled ||
                    (healthyOnly && details.healthStatus() != ServiceInstanceDetails::Healthy) ||
                    (!healthyOnly && details.healthStatus() == ServiceInstanceDetails::Healthy)) {
                    continue;
                }
                ArchitectureMetadata architecture;
                for (const auto &candidate : details.metadata().architectures()) {
                    if (candidate.id() == context.architectureId) {
                        architecture = candidate;
                        break;
                    }
                }
                if (architecture.id().isEmpty()) {
                    continue;
                }
                if (fallback && fallback->id().isEmpty()) {
                    *fallback = architecture;
                }
                const auto singers = details.metadata().singers();
                const bool hasEverySinger = std::ranges::all_of(context.singers, [&singers, &context](const SynthesisSinger &requested) {
                    return std::ranges::any_of(singers, [&requested, &context](const SingerMetadata &singer) {
                        return singer.id() == requested.id && singer.architectureId() == context.architectureId;
                    });
                });
                if (!hasEverySinger) {
                    continue;
                }
                return architecture;
            }
            return ArchitectureMetadata{};
        };
        ArchitectureMetadata healthyFallback;
        const auto healthy = findArchitecture(true, &healthyFallback);
        if (!healthy.id().isEmpty()) {
            return healthy;
        }
        ArchitectureMetadata waitingFallback;
        const auto waiting = findArchitecture(false, &waitingFallback);
        if (!waiting.id().isEmpty()) {
            return waiting;
        }
        return healthyFallback.id().isEmpty() ? waitingFallback : healthyFallback;
    }

    QStringList downstreamIndirectParameters(const ArchitectureMetadata &architecture, const QStringList &changedParameters) {
        const QSet<QString> editedParameters(changedParameters.cbegin(), changedParameters.cend());
        QSet<QString> affectedParameters = editedParameters;
        QSet<QString> requestedParameters;
        bool progressed = true;
        while (progressed) {
            progressed = false;
            for (const auto &metadata : architecture.parameters()) {
                if (metadata.kind() != ParameterMetadata::Indirect || editedParameters.contains(metadata.id()) || requestedParameters.contains(metadata.id())) {
                    continue;
                }
                const bool dependsOnAffectedParameter = std::ranges::any_of(metadata.dependsOn(), [&affectedParameters](const QString &dependency) {
                    return affectedParameters.contains(dependency);
                });
                if (!dependsOnAffectedParameter) {
                    continue;
                }
                requestedParameters.insert(metadata.id());
                affectedParameters.insert(metadata.id());
                progressed = true;
            }
        }

        QStringList result;
        for (const auto &metadata : architecture.parameters()) {
            if (requestedParameters.contains(metadata.id())) {
                result.append(metadata.id());
            }
        }
        return result;
    }

    std::optional<SynthesisContext> buildSynthesisContext(dspx::SingingClip *clip) {
        return buildContext(clip, nullptr);
    }

    ParameterConfiguration parameterConfiguration(const QString &architectureId, const QString &parameterId) {
        const auto service = SynthService::instance();
        if (!service) {
            return {};
        }
        for (const auto &configuration : service->allParameterConfigurations()) {
            if (configuration.id() == parameterId &&
                (configuration.architectureId().isEmpty() || configuration.architectureId() == architectureId)) {
                return configuration;
            }
        }
        return {};
    }

    SynthesisTaskType executableStage(const ArchitectureMetadata &architecture, SynthesisTaskType requestedStage) {
        if (requestedStage == SynthesisTaskType::Pronunciation && architecture.pronunciationMode() == QStringLiteral("SKIP")) {
            requestedStage = SynthesisTaskType::Phoneme;
        }
        if (requestedStage == SynthesisTaskType::Phoneme && architecture.phonemeMode() == QStringLiteral("SKIP")) {
            return SynthesisTaskType::Parameter;
        }
        if (requestedStage == SynthesisTaskType::Duration && architecture.phonemeMode() != QStringLiteral("FULL")) {
            return SynthesisTaskType::Parameter;
        }
        return requestedStage;
    }

    SynthesisTaskType nextStage(const ArchitectureMetadata &architecture, SynthesisTaskType completedStage) {
        switch (completedStage) {
            case SynthesisTaskType::Pronunciation:
                return executableStage(architecture, SynthesisTaskType::Phoneme);
            case SynthesisTaskType::Phoneme:
                return executableStage(architecture, SynthesisTaskType::Duration);
            case SynthesisTaskType::Duration:
                return SynthesisTaskType::Parameter;
            case SynthesisTaskType::Parameter:
                return SynthesisTaskType::Audio;
            case SynthesisTaskType::Audio:
                return SynthesisTaskType::Audio;
        }
        return SynthesisTaskType::Audio;
    }

    BuiltScore buildScore(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, const ArchitectureMetadata &architecture, bool forAudio, const std::optional<QStringList> &requestedParameters) {
        BuiltScore result;
        if (!window || !clip || !piece || !clip->sources()) {
            result.error = SynthesisProjectAddOn::tr("The synthesis piece is no longer attached to a valid clip.");
            return result;
        }
        auto timeline = window->projectTimeline()->musicTimeline();
        const double pieceStartTick = clip->start() + piece->position();
        const double pieceEndTick = pieceStartTick + piece->length();
        const double pieceStartSeconds = tickSeconds(timeline, pieceStartTick);
        const double pieceEndSeconds = tickSeconds(timeline, pieceEndTick);
        result.score.pieceDuration = std::max(0.0, pieceEndSeconds - pieceStartSeconds);
        result.score.mixSampleRate = configuredSampleRate(QStringLiteral("mixSampleRate"), 100.0);

        QList<dspx::Note *> notes;
        for (auto note : clip->notes()->asRange()) {
            const double noteTick = clip->start() + note->position();
            if (noteTick >= pieceStartTick && noteTick < pieceEndTick) {
                notes.append(note);
            }
        }
        std::sort(notes.begin(), notes.end(), [](dspx::Note *left, dspx::Note *right) {
            return left->position() < right->position();
        });
        double previousEnd = pieceStartSeconds;
        const int documentCentShift = globalCentShift(clip);
        for (auto note : notes) {
            const double noteStart = tickSeconds(timeline, clip->start() + note->position());
            const double noteEnd = tickSeconds(timeline, clip->start() + note->position() + note->length());
            if (noteStart + 1e-9 < previousEnd) {
                result.error = SynthesisProjectAddOn::tr("Some notes overlap. Move or resize the overlapping notes before synthesizing.");
                return result;
            }
            SynthesisScoreNote converted;
            converted.gap = std::max(0.0, noteStart - previousEnd);
            converted.duration = std::max(0.0, noteEnd - noteStart);
            converted.cent = std::clamp(note->keyNumber() * 100 + note->centShift() + documentCentShift, 0, 12800);
            converted.pronunciation = effectivePronunciation(note);
            converted.language = note->language();
            for (auto phoneme : effectivePhonemes(note)->asRange()) {
                converted.phonemes.append({
                    phoneme->token(),
                    phoneme->onset(),
                    phoneme->language(),
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
        for (int frame = 0; frame < mixFrames; ++frame) {
            const double seconds = pieceStartSeconds + frame / result.score.mixSampleRate;
            const double tick = timeline->create(seconds * 1000.0).totalTick() - clip->start();
            const auto rootWeights = rootWeightsAt(clip->sources(), tick);
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
            parameterIds = architecture.audioDependencies();
            if (parameterIds.isEmpty()) {
                for (const auto &metadata : architecture.parameters()) {
                    parameterIds.append(metadata.id());
                }
            }
        } else {
            if (requestedParameters) {
                result.score.requestedParameters = *requestedParameters;
            } else {
                for (const auto &metadata : architecture.parameters()) {
                    if (metadata.kind() == ParameterMetadata::Indirect) {
                        result.score.requestedParameters.append(metadata.id());
                    }
                }
            }
            result.score.requestedParameters.removeDuplicates();
            for (const auto &metadata : architecture.parameters()) {
                parameterIds.append(metadata.id());
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
            const auto configuration = parameterConfiguration(architecture.id(), id);
            const int fallback = configuration.id().isEmpty() ? 0 : configuration.defaultValue();
            const SynthesisParameterEvaluator evaluator(clip->parameters()->item(id), *minimumTick, *maximumTick);
            for (const int relativeTick : parameterTicks) {
                const int defaultValue = id == QStringLiteral("pitch") ? pitchAt(clip, relativeTick) : fallback;
                double value = evaluator.evaluate(relativeTick, defaultValue);
                if (id == QStringLiteral("pitch")) {
                    value = std::clamp(value + documentCentShift, 0.0, 12800.0);
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
        const double pieceEnd = piecePosition + pieceLength;
        QList<dspx::Note *> notes;
        for (auto note : clip->notes()->asRange()) {
            if (note->position() < piecePosition || note->position() >= pieceEnd) {
                continue;
            }
            notes.append(note);
        }
        std::sort(notes.begin(), notes.end(), [](dspx::Note *left, dspx::Note *right) {
            return left->position() < right->position();
        });
        for (auto note : notes) {
            result.noteHandles.append(note->handle());
            if (type == SynthesisTaskType::Pronunciation) {
                result.request.lyricNotes.append({note->lyric(), note->language()});
            } else {
                result.request.pronunciationNotes.append({effectivePronunciation(note), note->language()});
            }
        }
        return result;
    }

    double tickSeconds(SVS::MusicTimeline *timeline, double tick) {
        return timeline->create(0, 0, static_cast<int>(std::round(tick))).millisecond() / 1000.0;
    }

}
