#include "SynthesisProjectAddOn.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMutex>
#include <QPointer>
#include <QPromise>
#include <QQmlComponent>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QThread>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <opendspx/anchornode.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/SVSCraftNamespace.h>

#include <TalcsCore/FutureAudioSource.h>
#include <TalcsCore/FutureAudioSourceClipSeries.h>
#include <TalcsCore/PositionableMixerAudioSource.h>
#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/AudioFormatInputSource.h>
#include <TalcsFormat/FormatManager.h>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <dspxmodelCore/Document.h>
#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
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
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelPiece/ClipChange.h>
#include <dspxmodelPiece/ClipChangeRange.h>
#include <dspxmodelPiece/ClipWatcher.h>
#include <dspxmodelPiece/Piece.h>
#include <dspxmodelPiece/PieceDivider.h>
#include <opendspxinterpolator/parameterinterpolator.h>
#include <transactional/TransactionController.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <audio/AudioExporter.h>
#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <audio/TrackAudioContext.h>
#include <synth/ProjectSynthesisContext.h>
#include <synth/ServiceTypes.h>
#include <synth/SynthInterface.h>
#include <synth/SynthesisPiece.h>
#include <synth/SynthesisTask.h>
#include <synth/SynthesisTaskManager.h>
#include <synth/internal/SynthService.h>
#include <synth/internal/private/ParameterExpressionUtils_p.h>
#include <synth/private/ProjectSynthesisContext_p.h>
#include <synth/private/SynthesisPiece_p.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcSynthesisScheduler, "diffscope.synth.scheduler")

    namespace {

        double configuredSampleRate(const QString &key, double fallback) {
            auto settings = Core::RuntimeInterface::settings();
            settings->beginGroup(QStringLiteral("org.diffscope.synth"));
            const double value = settings->value(key, fallback).toDouble();
            settings->endGroup();
            return std::max(1.0, value);
        }

        void configureDivider(dspx::PieceDivider *divider) {
            if (!divider) {
                return;
            }
            auto settings = Core::RuntimeInterface::settings();
            settings->beginGroup(QStringLiteral("org.diffscope.synth"));
            divider->setPaddingBase(settings->value(QStringLiteral("piecePaddingBaseMs"), 100.0).toDouble());
            divider->setPaddingAdditional(settings->value(
                                                      QStringLiteral("piecePaddingAdditionalMs"),
                                                      100.0
            )
                                              .toDouble());
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

        struct FlattenedSinger {
            SynthesisSinger singer;
            int rootIndex{};
            double nestedWeight{};
        };

        struct BuiltScore {
            SynthesisScore score;
            QList<dspx::Handle> noteHandles;
            QString error;
        };

        struct BuiltLanguageRequest {
            SynthesisTaskRequest request;
            QList<dspx::Handle> noteHandles;
        };

        bool rangesOverlap(double leftPosition, double leftLength, double rightPosition, double rightLength) {
            return leftPosition < rightPosition + rightLength &&
                   rightPosition < leftPosition + leftLength;
        }

        bool sameSynthesisInput(SynthesisTaskRequest left, SynthesisTaskRequest right) {
            left.displayName.clear();
            right.displayName.clear();
            // The duration endpoint does not consume parameter curves even though they are
            // present in the shared score model.
            if (left.type == SynthesisTaskType::Duration) {
                left.score.parameters.clear();
                left.score.requestedParameters.clear();
                right.score.parameters.clear();
                right.score.requestedParameters.clear();
            }
            return left == right;
        }

        std::vector<double> logicalWeights(const QList<double> &stored, int count) {
            if (count <= 0) {
                return {};
            }
            std::vector<double> result(static_cast<std::size_t>(count), 0.0);
            double sum{};
            for (int index = 0; index < count - 1; ++index) {
                const double value = index < stored.size()
                                         ? std::clamp(stored.at(index), 0.0, 1.0)
                                         : 0.0;
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

        ArchitectureMetadata architectureFor(const SynthesisContext &context) {
            auto interface = SynthInterface::instance();
            if (!interface) {
                return {};
            }
            for (const auto &service : interface->serviceInstances()) {
                const auto details = interface->serviceInstanceDetails(service.id());
                if (!service.isEnabled() || details.healthStatus() != ServiceInstanceDetails::Healthy) {
                    continue;
                }
                const auto singers = details.metadata().singers();
                const bool hasEverySinger = std::ranges::all_of(
                    context.singers, [&singers, &context](const SynthesisSinger &requested) {
                        return std::ranges::any_of(singers, [&requested, &context](const SingerMetadata &singer) {
                            return singer.id() == requested.id &&
                                   singer.architectureId() == context.architectureId;
                        });
                    }
                );
                if (!hasEverySinger) {
                    continue;
                }
                for (const auto &architecture : details.metadata().architectures()) {
                    if (architecture.id() == context.architectureId) {
                        return architecture;
                    }
                }
            }
            return {};
        }

        QStringList downstreamIndirectParameters(const ArchitectureMetadata &architecture, const QStringList &changedParameters) {
            const QSet<QString> editedParameters(changedParameters.cbegin(), changedParameters.cend());
            QSet<QString> affectedParameters = editedParameters;
            QSet<QString> requestedParameters;
            bool progressed = true;
            while (progressed) {
                progressed = false;
                for (const auto &metadata : architecture.parameters()) {
                    if (metadata.kind() != ParameterMetadata::Indirect ||
                        editedParameters.contains(metadata.id()) ||
                        requestedParameters.contains(metadata.id())) {
                        continue;
                    }
                    const bool dependsOnAffectedParameter = std::ranges::any_of(
                        metadata.dependsOn(), [&affectedParameters](const QString &dependency) {
                            return affectedParameters.contains(dependency);
                        }
                    );
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

        std::optional<SynthesisContext> buildSynthesisContext(dspx::SingingClip *clip, QList<FlattenedSinger> *flattened = nullptr) {
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
            return note->editedPhonemes()->size() > 0 ? note->editedPhonemes()
                                                      : note->originalPhonemes();
        }

        double tickSeconds(SVS::MusicTimeline *timeline, double tick) {
            return timeline->create(0, 0, static_cast<int>(std::round(tick))).millisecond() / 1000.0;
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
            if (!left)
                left = anchors->firstItem();
            if (!right)
                right = anchors->lastItem();
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

        ParameterConfiguration parameterConfiguration(const QString &architectureId, const QString &parameterId) {
            const auto service = SynthService::instance();
            if (!service) {
                return {};
            }
            for (const auto &configuration : service->allParameterConfigurations()) {
                if (configuration.id() == parameterId &&
                    (configuration.architectureId().isEmpty() ||
                     configuration.architectureId() == architectureId)) {
                    return configuration;
                }
            }
            return {};
        }

        struct AnchorCurveSegment {
            int firstTick{};
            int lastTick{};
            opendspx::ParameterInterpolator interpolator;
        };

        std::vector<AnchorCurveSegment> buildAnchorCurve(const dspx::AnchorNodeSequence *sequence) {
            std::vector<AnchorCurveSegment> result;
            if (!sequence) {
                return result;
            }
            std::vector<opendspx::AnchorNode> current;
            const auto appendSegment = [&result, &current] {
                if (current.empty()) {
                    return;
                }
                const int firstTick = current.front().x;
                const int lastTick = current.back().x;
                result.push_back({
                    firstTick,
                    lastTick,
                    opendspx::ParameterInterpolator(std::move(current)),
                });
                current.clear();
            };
            for (const auto node : sequence->asRange()) {
                current.push_back({
                    static_cast<opendspx::AnchorNode::Interpolation>(node->interpolationMode()),
                    node->x(),
                    node->y(),
                });
                if (node->interpolationMode() == dspx::AnchorNode::None) {
                    appendSegment();
                }
            }
            appendSegment();
            return result;
        }

        struct FreeValueSpan {
            int firstIndex{};
            QList<QVariant> values;
        };

        FreeValueSpan captureFreeValues(const dspx::FreeValueDataArray *array, int minimumTick, int maximumTick) {
            if (!array || maximumTick < 0 || maximumTick < minimumTick) {
                return {};
            }
            const int firstIndex = std::max(0, minimumTick / dspx::FreeValueDataArray::step());
            const int lastIndex = maximumTick / dspx::FreeValueDataArray::step();
            if (lastIndex < firstIndex) {
                return {firstIndex, {}};
            }
            const qint64 requestedLength = static_cast<qint64>(lastIndex) - firstIndex + 2;
            return {firstIndex, array->slice(firstIndex, static_cast<int>(requestedLength))};
        }

        std::optional<double> freeValue(const FreeValueSpan &span, double tick) {
            if (tick < 0.0) {
                return std::nullopt;
            }
            const double index = tick / dspx::FreeValueDataArray::step();
            const int leftIndex = static_cast<int>(std::floor(index));
            const int offset = leftIndex - span.firstIndex;
            if (offset < 0 || offset >= span.values.size()) {
                return std::nullopt;
            }
            const auto &leftValue = span.values.at(offset);
            if (!leftValue.isValid() || leftValue.isNull()) {
                return std::nullopt;
            }
            const double left = leftValue.toDouble();
            const double fraction = index - leftIndex;
            if (qFuzzyIsNull(fraction)) {
                return left;
            }
            if (offset + 1 >= span.values.size()) {
                return std::nullopt;
            }
            const auto &rightValue = span.values.at(offset + 1);
            if (!rightValue.isValid() || rightValue.isNull()) {
                return std::nullopt;
            }
            return left + (rightValue.toDouble() - left) * fraction;
        }

        std::optional<double> anchorValue(const std::vector<AnchorCurveSegment> &segments, double tick) {
            const auto segment = std::lower_bound(
                segments.cbegin(), segments.cend(), tick,
                [](const AnchorCurveSegment &candidate, double value) {
                    return candidate.lastTick < value;
                }
            );
            if (segment == segments.cend() || tick < segment->firstTick) {
                return std::nullopt;
            }
            return segment->interpolator.evaluate(tick);
        }

        class ParameterValueEvaluator {
        public:
            ParameterValueEvaluator(dspx::Parameter *parameter, int minimumTick, int maximumTick)
                : m_parameter(parameter),
                  m_minimumTick(minimumTick),
                  m_maximumTick(maximumTick),
                  m_editedAnchors(buildAnchorCurve(parameter ? parameter->anchorEdited() : nullptr)),
                  m_transformAnchors(buildAnchorCurve(parameter ? parameter->anchorTransform() : nullptr)),
                  m_editedArray(parameter ? parameter->freeEdited() : nullptr),
                  m_originalArray(parameter ? parameter->original() : nullptr),
                  m_transformArray(parameter ? parameter->freeTransform() : nullptr) {
            }

            double evaluate(double tick, double fallback) const {
                if (!m_parameter) {
                    return fallback;
                }
                auto base = anchorValue(m_editedAnchors, tick);
                if (!base) {
                    base = freeValue(values(m_editedArray, m_editedValues), tick);
                }
                if (!base) {
                    base = freeValue(values(m_originalArray, m_originalValues), tick);
                }
                if (!base) {
                    base = fallback;
                }
                auto transform = anchorValue(m_transformAnchors, tick);
                if (!transform) {
                    transform = freeValue(values(m_transformArray, m_transformValues), tick);
                }
                return *base * (transform ? *transform / 1000.0 : 1.0);
            }

        private:
            const FreeValueSpan &values(
                const dspx::FreeValueDataArray *array,
                std::optional<FreeValueSpan> &cached
            ) const {
                if (!cached) {
                    cached = captureFreeValues(array, m_minimumTick, m_maximumTick);
                }
                return *cached;
            }

            dspx::Parameter *m_parameter{};
            int m_minimumTick{};
            int m_maximumTick{};
            std::vector<AnchorCurveSegment> m_editedAnchors;
            std::vector<AnchorCurveSegment> m_transformAnchors;
            const dspx::FreeValueDataArray *m_editedArray{};
            const dspx::FreeValueDataArray *m_originalArray{};
            const dspx::FreeValueDataArray *m_transformArray{};
            mutable std::optional<FreeValueSpan> m_editedValues;
            mutable std::optional<FreeValueSpan> m_originalValues;
            mutable std::optional<FreeValueSpan> m_transformValues;
        };

        double normalizedParameterValue(const ParameterValueEvaluator &evaluator, const ParameterConfiguration &configuration, double relativeTick, double fallback) {
            const double value = evaluator.evaluate(relativeTick, fallback);
            double normalized = value;
            if (!configuration.id().isEmpty() &&
                !ParameterExpressionUtils::evaluate(configuration.normalizationExpression(), value, &normalized)) {
                normalized = value;
            }
            return normalized;
        }

        int pitchAt(dspx::SingingClip *clip, int relativeTick) {
            for (auto note : clip->notes()->asRange()) {
                if (relativeTick >= note->position() &&
                    relativeTick < note->position() + note->length()) {
                    return note->keyNumber() * 100 + note->centShift();
                }
            }
            return 0;
        }

        BuiltScore buildScore(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, const ArchitectureMetadata &architecture, bool forAudio, const std::optional<QStringList> &requestedParameters = std::nullopt) {
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
                converted.cent = std::clamp(note->keyNumber() * 100 + note->centShift(), 0, 12800);
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
            buildSynthesisContext(clip, &leaves);
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
                    for (double &value : weights)
                        value /= sum;
                }
                if (!weights.isEmpty())
                    weights.removeLast();
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
            const auto [minimumTick, maximumTick] = std::minmax_element(
                parameterTicks.cbegin(), parameterTicks.cend()
            );
            for (const auto &id : parameterIds) {
                SynthesisParameter parameter;
                parameter.sampleRate = parameterSampleRate;
                const auto configuration = parameterConfiguration(architecture.id(), id);
                const int fallback = configuration.id().isEmpty() ? 0 : configuration.defaultValue();
                const ParameterValueEvaluator evaluator(
                    clip->parameters()->item(id), *minimumTick, *maximumTick
                );
                for (const int relativeTick : parameterTicks) {
                    const int defaultValue = id == QStringLiteral("pitch")
                                                 ? pitchAt(clip, relativeTick)
                                                 : fallback;
                    parameter.values.append(normalizedParameterValue(evaluator, configuration, relativeTick, defaultValue));
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
            // Language tasks keep the piece range captured when they are created. A
            // later phoneme result may change the divider, but must not widen this request.
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

        void replaceOriginalPhonemes(dspx::Note *note, const QList<SynthesisPhoneme> &phonemes) {
            auto model = note->model();
            const auto old = note->originalPhonemes()->asRange();
            QList<dspx::Phoneme *> toRemove;
            for (auto phoneme : old)
                toRemove.append(phoneme);
            for (auto phoneme : toRemove)
                model->destroyItem(phoneme);
            for (const auto &source : phonemes) {
                auto phoneme = model->createOriginalPhoneme();
                phoneme->setToken(source.token);
                phoneme->setOnset(source.onset);
                phoneme->setLanguage(source.language);
                phoneme->setStart(static_cast<int>(std::round(source.start * 1000.0)));
                note->originalPhonemes()->insertItem(phoneme);
            }
        }

        void writeParameterOrigins(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, const QMap<QString, SynthesisParameter> &parameters) {
            auto timeline = window->projectTimeline()->musicTimeline();
            const int firstIndex = std::max(0, static_cast<int>(std::floor(piece->position() / dspx::FreeValueDataArray::step())));
            const int lastIndex = std::max(firstIndex, static_cast<int>(std::ceil((piece->position() + piece->length()) / dspx::FreeValueDataArray::step())));
            const double pieceStartSeconds = tickSeconds(timeline, clip->start() + piece->position());
            for (auto it = parameters.cbegin(); it != parameters.cend(); ++it) {
                auto modelParameter = clip->parameters()->item(it.key());
                if (!modelParameter || it->values.isEmpty() || it->sampleRate <= 0.0) {
                    continue;
                }
                const auto configuration = parameterConfiguration(clip->sources()->category(), it.key());
                QList<QVariant> values;
                for (int index = firstIndex; index < lastIndex; ++index) {
                    const int tick = index * dspx::FreeValueDataArray::step();
                    const double offset = tickSeconds(timeline, clip->start() + tick) - pieceStartSeconds;
                    const int sample = std::clamp(static_cast<int>(std::round(offset * it->sampleRate)), 0, static_cast<int>(it->values.size()) - 1);
                    double value = it->values.at(sample);
                    if (!configuration.id().isEmpty()) {
                        double denormalized{};
                        if (ParameterExpressionUtils::evaluate(configuration.denormalizationExpression(), value, &denormalized)) {
                            value = denormalized;
                        }
                    }
                    values.append(static_cast<int>(std::round(value)));
                }
                auto original = modelParameter->original();
                if (original->size() < firstIndex) {
                    original->splice(original->size(), 0, QList<QVariant>(firstIndex - original->size()));
                }
                const int removed = std::min(static_cast<int>(values.size()), original->size() - firstIndex);
                original->splice(firstIndex, std::max(0, removed), values);
            }
        }

        void clearParameterOrigins(dspx::SingingClip *clip, const QList<QPair<double, double>> &ranges, const std::optional<QStringList> &parameterIds) {
            if (!clip || ranges.isEmpty() || (parameterIds && parameterIds->isEmpty())) {
                return;
            }
            const QSet<QString> selected = parameterIds
                                               ? QSet<QString>(parameterIds->cbegin(), parameterIds->cend())
                                               : QSet<QString>{};
            const auto keys = clip->parameters()->keys();
            for (const auto &key : keys) {
                if (parameterIds && !selected.contains(key)) {
                    continue;
                }
                auto parameter = clip->parameters()->item(key);
                auto original = parameter ? parameter->original() : nullptr;
                if (!original || original->size() == 0) {
                    continue;
                }
                for (const auto &[position, length] : ranges) {
                    const int firstIndex = std::max(0, static_cast<int>(std::floor(position / dspx::FreeValueDataArray::step())));
                    const int lastIndex = std::max(firstIndex, static_cast<int>(std::ceil((position + length) / dspx::FreeValueDataArray::step())));
                    if (firstIndex >= original->size()) {
                        continue;
                    }
                    const int count = std::min(lastIndex - firstIndex, original->size() - firstIndex);
                    if (count > 0) {
                        original->splice(firstIndex, count, QList<QVariant>(count));
                    }
                }
            }
        }

        struct AudioClipRange {
            qint64 position{};
            qint64 sourceStart{};
            qint64 length{};

            bool isValid() const {
                return length > 0;
            }
        };

        AudioClipRange audioClipRange(Core::ProjectWindowInterface *window, Audio::TrackAudioContext *trackContext, dspx::SingingClip *clip, SynthesisPiece *piece) {
            if (!window || !trackContext || !clip || !piece) {
                return {};
            }
            auto timeline = window->projectTimeline()->musicTimeline();
            const double pieceStartTick = clip->start() + piece->position();
            const double pieceEndTick = pieceStartTick + piece->length();
            const double visibleStartTick = clip->position();
            const double visibleEndTick = visibleStartTick + clip->clipLength();
            const double startTick = std::max(pieceStartTick, visibleStartTick);
            const double endTick = std::min(pieceEndTick, visibleEndTick);
            if (endTick <= startTick) {
                return {};
            }
            double sampleRate = trackContext->trackMixer()->sampleRate();
            if (qFuzzyIsNull(sampleRate)) {
                sampleRate = Audio::GlobalAudioContext::sampleRate();
            }
            const double pieceStartSeconds = tickSeconds(timeline, pieceStartTick);
            const double startSeconds = tickSeconds(timeline, startTick);
            const double endSeconds = tickSeconds(timeline, endTick);
            return {
                static_cast<qint64>(std::llround(startSeconds * sampleRate)),
                static_cast<qint64>(std::llround((startSeconds - pieceStartSeconds) * sampleRate)),
                std::max<qint64>(1, static_cast<qint64>(std::llround((endSeconds - startSeconds) * sampleRate))),
            };
        }

    }

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

    struct SynthesisProjectAddOn::AudioBinding {
        talcs::FutureAudioSourceClipSeries *series{};
        talcs::FutureAudioSourceClipSeries::ClipView clip;
        talcs::FutureAudioSource *futureSource{};
        std::shared_ptr<QPromise<talcs::PositionableAudioSource *>> promise;
        talcs::PositionableMixerAudioSource *mixer{};
        talcs::AudioFormatInputSource *source{};
        AudioClipRange range;
    };

    class SynthesisExportListener final : public Audio::AudioExporterListener {
    public:
        static SynthesisExportListener &instance() {
            static SynthesisExportListener listener;
            return listener;
        }

        void attach(Core::ProjectWindowInterface *window, SynthesisProjectAddOn *addOn) {
            QMutexLocker locker(&m_mutex);
            m_addOns.insert(window, addOn);
        }

        void detach(Core::ProjectWindowInterface *window, SynthesisProjectAddOn *addOn) {
            QMutexLocker locker(&m_mutex);
            if (m_addOns.value(window) == addOn) {
                m_addOns.remove(window);
            }
        }

        bool willStartCallback(Audio::AudioExporter *exporter) override {
            return waitForSynthesis(exporter);
        }

        void willFinishCallback(Audio::AudioExporter *) override {
        }

    private:
        SynthesisExportListener() {
            Audio::AudioExporter::registerListener(this);
        }

        bool waitForSynthesis(Audio::AudioExporter *exporter) {
            QPointer<SynthesisProjectAddOn> addOn;
            {
                QMutexLocker locker(&m_mutex);
                addOn = m_addOns.value(exporter ? exporter->windowHandle() : nullptr);
            }
            if (!addOn) {
                return true;
            }
            bool accepted{};
            QString errorMessage;
            const auto wait = [addOn, &accepted, &errorMessage] {
                if (addOn) {
                    accepted = addOn->waitForAudioSynthesis(&errorMessage);
                }
            };
            if (QThread::currentThread() == addOn->thread()) {
                wait();
            } else if (!QMetaObject::invokeMethod(addOn, wait, Qt::BlockingQueuedConnection)) {
                errorMessage = SynthesisProjectAddOn::tr("The synthesis state could not be checked before audio export.");
                accepted = false;
            }
            if (!accepted && exporter) {
                exporter->cancel(true, errorMessage.isEmpty()
                                           ? SynthesisProjectAddOn::tr("Audio synthesis did not complete successfully.")
                                           : errorMessage);
            }
            return accepted;
        }

        QMutex m_mutex;
        QHash<Core::ProjectWindowInterface *, QPointer<SynthesisProjectAddOn>> m_addOns;
    };

    SynthesisProjectAddOn::SynthesisProjectAddOn(QObject *parent)
        : Core::WindowInterfaceAddOn(parent) {
    }

    SynthesisProjectAddOn::~SynthesisProjectAddOn() {
        m_subscription.disconnect();
        auto window = windowHandle() ? windowHandle()->cast<Core::ProjectWindowInterface>() : nullptr;
        SynthesisExportListener::instance().detach(window, this);
        if (m_context) {
            ProjectSynthesisContextPrivate::get(m_context)->controller = nullptr;
        }
        cancelAll();
        const auto clips = m_clips.keys();
        for (auto clip : clips) {
            removeClip(clip);
        }
        qDeleteAll(m_retiredClips);
        qDeleteAll(m_taskWritebacks);
        qDeleteAll(m_pendingManualRequests);
    }

    void SynthesisProjectAddOn::initialize() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        m_taskManager = SynthInterface::instance()->taskManager();
        m_context = new ProjectSynthesisContext(window, this);
        ProjectSynthesisContextPrivate::get(m_context)->controller = this;
        window->addObject(m_context);
        auto document = window->projectDocumentContext()->document();
        m_transactionController = document->transactionController();
        if (m_transactionController) {
            connect(m_transactionController.data(), &Core::TransactionController::transactionActiveChanged,
                    this, [this](bool active) {
                if (!active)
                    schedulePendingWork();
            });
            connect(m_transactionController.data(), &Core::TransactionController::transactionAborted,
                    this, [this] {
                m_documentSyncPending = true;
                m_rollbackPending = true;
                schedulePendingWork();
            });
        }
        if (auto service = SynthService::instance()) {
            connect(service, &SynthService::managedArchitecturesChanged, this, [this] {
                m_architectureSyncPending = true;
                schedulePendingWork();
            });
        }
        SynthesisExportListener::instance().attach(window, this);

        QQmlComponent actions(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("SynthesisProjectActions"), this);
        if (actions.isError())
            qFatal() << actions.errorString();
        auto actionObject = actions.createWithInitialProperties({
            {QStringLiteral("addOn"), QVariant::fromValue(this)},
        });
        if (!actionObject)
            qFatal() << actions.errorString();
        actionObject->setParent(this);
        QMetaObject::invokeMethod(actionObject, "registerToContext", window->actionContext());

        auto engine = document->model()->document()->engine();
        m_subscription = engine->subscribe([this](const dini::EngineEvent &event) {
            if (event.kind != dini::EventKind::AfterCommit || m_internalCommit) {
                return;
            }
            m_commitPending = true;
            schedulePendingWork();
        });
        connect(window->projectTimeline(), &Core::ProjectTimeline::positionChanged, this, &SynthesisProjectAddOn::updatePriorities);
        if (auto audioContext = Audio::ProjectAudioContext::of(window)) {
            connect(audioContext, &Audio::ProjectAudioContext::statusChanged, this, &SynthesisProjectAddOn::updatePriorities);
        }
        synchronizeDocument();
        QTimer::singleShot(0, this, [this] {
            resynthesizeProject(SynthesisTaskType::Pronunciation, {});
        });
    }

    void SynthesisProjectAddOn::extensionsInitialized() {
    }

    bool SynthesisProjectAddOn::delayedInitialize() {
        return Core::WindowInterfaceAddOn::delayedInitialize();
    }

    ProjectSynthesisContext *SynthesisProjectAddOn::synthesisContext() const {
        return m_context;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::pieces() const {
        QList<SynthesisPiece *> result;
        for (auto runtime : m_clips) {
            if (!runtime || !runtime->clip) {
                continue;
            }
            for (auto piece : runtime->pieces) {
                if (piece && piece->singingClip()) {
                    result.append(piece);
                }
            }
        }
        std::sort(result.begin(), result.end(), [](SynthesisPiece *left, SynthesisPiece *right) {
            const auto leftClip = left ? left->singingClip() : nullptr;
            const auto rightClip = right ? right->singingClip() : nullptr;
            if (!leftClip || !rightClip) {
                return leftClip != nullptr;
            }
            const double leftStart = leftClip->start() + left->position();
            const double rightStart = rightClip->start() + right->position();
            return leftStart < rightStart;
        });
        return result;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::piecesForClip(dspx::SingingClip *clip) const {
        const auto runtime = runtimeForClip(clip);
        if (!runtime || !runtime->divider)
            return {};
        QList<SynthesisPiece *> result;
        for (auto piece : runtime->divider->pieces()) {
            auto synthesisPiece = runtime->pieces.value(piece);
            if (synthesisPiece && synthesisPiece->singingClip())
                result.append(synthesisPiece);
        }
        return result;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::piecesInRange(dspx::SingingClip *clip, double position, double length) const {
        const auto runtime = runtimeForClip(clip);
        if (!runtime || !runtime->divider)
            return {};
        QList<SynthesisPiece *> result;
        for (auto piece : runtime->divider->slice(position, length)) {
            auto synthesisPiece = runtime->pieces.value(piece);
            if (synthesisPiece && synthesisPiece->singingClip())
                result.append(synthesisPiece);
        }
        return result;
    }

    int SynthesisProjectAddOn::synthesizingPieceCount() const {
        return static_cast<int>(std::ranges::count_if(pieces(), [](SynthesisPiece *piece) {
            return piece->state() == SynthesisPiece::Synthesizing;
        }));
    }

    int SynthesisProjectAddOn::queuedPieceCount() const {
        return static_cast<int>(std::ranges::count_if(pieces(), [](SynthesisPiece *piece) {
            return piece->state() == SynthesisPiece::Queued;
        }));
    }

    QList<dspx::SingingClip *> SynthesisProjectAddOn::synchronizeDocument() {
        if (documentTransactionActive()) {
            m_documentSyncPending = true;
            schedulePendingWork();
            return {};
        }
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto model = window->projectDocumentContext()->document()->model();
        QSet<dspx::Handle> live;
        QList<dspx::SingingClip *> added;
        for (auto track : model->tracks()->items()) {
            for (auto clip : track->clips()->asRange()) {
                if (clip->type() == dspx::Clip::Singing) {
                    auto singing = static_cast<dspx::SingingClip *>(clip);
                    if (!isManagedClip(singing))
                        continue;
                    const auto handle = singing->handle();
                    live.insert(handle);
                    if (auto runtime = m_clips.value(handle)) {
                        if (runtime->clip != singing)
                            rebindClip(runtime, singing);
                    } else {
                        addClip(singing);
                        added.append(singing);
                    }
                }
            }
        }
        for (const auto handle : m_clips.keys()) {
            if (!live.contains(handle))
                removeClip(handle);
        }
        return added;
    }

    SynthesisProjectAddOn::ClipRuntime *SynthesisProjectAddOn::runtimeForClip(dspx::SingingClip *clip) const {
        if (!clip)
            return nullptr;
        auto runtime = m_clips.value(clip->handle());
        return runtime && runtime->clip == clip ? runtime : nullptr;
    }

    SynthesisProjectAddOn::ClipRuntime *SynthesisProjectAddOn::runtimeForPiece(SynthesisPiece *piece) const {
        if (!piece)
            return nullptr;
        for (auto runtime : m_clips) {
            if (runtime && runtime->pieces.values().contains(piece))
                return runtime;
        }
        return nullptr;
    }

    void SynthesisProjectAddOn::addClip(dspx::SingingClip *clip) {
        if (!clip || m_clips.contains(clip->handle())) {
            return;
        }
        auto runtime = new ClipRuntime;
        runtime->clipHandle = clip->handle();
        runtime->clip = clip;
        runtime->divider = new dspx::PieceDivider(this);
        runtime->watcher = new dspx::ClipWatcher(this);
        configureDivider(runtime->divider);
        runtime->divider->setSingingClip(clip);
        runtime->watcher->setSingingClip(clip);
        runtime->divider->update();
        m_clips.insert(runtime->clipHandle, runtime);
        watchClipLifetime(runtime);
        synchronizePieces(runtime);
    }

    void SynthesisProjectAddOn::rebindClip(ClipRuntime *runtime, dspx::SingingClip *clip) {
        if (!runtime || !clip || runtime->clipHandle != clip->handle() || runtime->clip == clip)
            return;
        runtime->clip = clip;
        runtime->rebound = true;
        for (auto piece : runtime->pieces) {
            if (piece)
                SynthesisPiecePrivate::get(piece)->clip = clip;
        }
        configureDivider(runtime->divider);
        runtime->divider->setSingingClip(clip);
        runtime->watcher->setSingingClip(clip);
        runtime->divider->update();
        synchronizePieces(runtime);
        for (auto piece : runtime->pieces) {
            auto binding = m_audioBindings.value(piece);
            auto mixer = binding ? binding->mixer : nullptr;
            if (!mixer)
                continue;
            mixer->setGain(static_cast<float>(clip->gain()));
            mixer->setPan(static_cast<float>(clip->pan()));
            mixer->setSilentFlags(clip->mute() ? -1 : 0);
            connect(clip, &dspx::Clip::gainChanged, mixer, &talcs::PositionableMixerAudioSource::setGain);
            connect(clip, &dspx::Clip::panChanged, mixer, &talcs::PositionableMixerAudioSource::setPan);
            connect(clip, &dspx::Clip::muteChanged, mixer, [mixer](bool mute) {
                mixer->setSilentFlags(mute ? -1 : 0);
            });
        }
        watchClipLifetime(runtime);
    }

    void SynthesisProjectAddOn::watchClipLifetime(ClipRuntime *runtime) {
        if (!runtime || !runtime->clip)
            return;
        auto watchedClip = runtime->clip.data();
        connect(watchedClip, &QObject::destroyed, this, [this, runtime, watchedClip] {
            if (m_clips.value(runtime->clipHandle) != runtime)
                return;
            if (runtime->clip && runtime->clip != watchedClip)
                return;
            runtime->clip = nullptr;
            m_documentSyncPending = true;
            schedulePendingWork();
        });
    }

    void SynthesisProjectAddOn::resetClipBaseline(ClipRuntime *runtime) {
        if (!runtime || !runtime->clip || !runtime->divider || !runtime->watcher)
            return;
        runtime->divider->setSingingClip(nullptr);
        runtime->divider->setSingingClip(runtime->clip);
        runtime->divider->update();
        synchronizePieces(runtime);
        runtime->watcher->setSingingClip(nullptr);
        runtime->watcher->setSingingClip(runtime->clip);
        runtime->rebound = false;
    }

    void SynthesisProjectAddOn::removeClip(dspx::Handle clipHandle) {
        auto runtime = m_clips.take(clipHandle);
        if (!runtime)
            return;
        runtime->clip = nullptr;
        for (auto piece : runtime->pieces) {
            if (!piece) {
                continue;
            }
            auto d = SynthesisPiecePrivate::get(piece);
            d->clip = nullptr;
            ++d->revision;
            if (auto task = m_pieceTasks.value(piece); task && task->state() == SynthesisTask::Queued) {
                discardTaskWritebacks(task);
                m_taskManager->cancel(task);
            }
            m_pieceTasks.remove(piece);
            removeAudio(piece);
            piece->deleteLater();
        }
        runtime->pieces.clear();
        detachAudioSeries(runtime);
        delete runtime->divider;
        delete runtime->watcher;
        runtime->divider = nullptr;
        runtime->watcher = nullptr;
        m_retiredClips.append(runtime);
        Q_EMIT m_context->piecesChanged();
        updateCounts();
    }

    void SynthesisProjectAddOn::synchronizePieces(ClipRuntime *runtime) {
        if (!runtime || !runtime->clip || !runtime->divider) {
            return;
        }
        const auto sourcePieces = runtime->divider->pieces();
        QSet<dspx::Piece *> live(sourcePieces.cbegin(), sourcePieces.cend());
        bool changed{};
        for (auto source : sourcePieces) {
            auto piece = runtime->pieces.value(source);
            if (!piece) {
                piece = new SynthesisPiece(runtime->clip, m_context);
                runtime->pieces.insert(source, piece);
                connect(piece, &QObject::destroyed, this, [this, piece] {
                    m_pieceTasks.remove(piece);
                    destroyAudioBinding(m_audioBindings.take(piece));
                });
                changed = true;
            }
            auto d = SynthesisPiecePrivate::get(piece);
            if (d->position != source->position() || d->length != source->length()) {
                d->position = source->position();
                d->length = source->length();
                Q_EMIT piece->rangeChanged();
            }
        }
        for (auto source : runtime->pieces.keys()) {
            if (live.contains(source))
                continue;
            auto piece = runtime->pieces.take(source);
            if (!piece) {
                continue;
            }
            auto d = SynthesisPiecePrivate::get(piece);
            d->clip = nullptr;
            ++d->revision;
            // A language task owns an immutable range and remains useful when a
            // phoneme writeback replaces its SynthesisPiece projection.
            if (auto task = m_pieceTasks.value(piece);
                task && task->state() == SynthesisTask::Queued &&
                task->type() > SynthesisTaskType::Phoneme) {
                discardTaskWritebacks(task);
                m_taskManager->cancel(task);
            }
            m_pieceTasks.remove(piece);
            removeAudio(piece);
            piece->deleteLater();
            changed = true;
        }
        for (auto piece : runtime->pieces) {
            if (piece && !isPieceInSynthesisRange(runtime, piece)) {
                deactivatePiece(piece);
            }
        }
        if (changed)
            Q_EMIT m_context->piecesChanged();
        updateCounts();
    }

    bool SynthesisProjectAddOn::isPieceInSynthesisRange(const ClipRuntime *runtime, const SynthesisPiece *piece) const {
        if (!runtime || !runtime->clip || !piece || piece->singingClip() != runtime->clip) {
            return false;
        }
        const double visibleStart = runtime->clip->clipStart();
        const double visibleEnd = visibleStart + runtime->clip->clipLength();
        return piece->position() + piece->length() > visibleStart &&
               piece->position() < visibleEnd;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::synthesisPiecesForClip(const ClipRuntime *runtime) const {
        if (!runtime || !runtime->clip || !runtime->divider) {
            return {};
        }
        QList<SynthesisPiece *> result;
        for (auto source : runtime->divider->pieces()) {
            auto piece = runtime->pieces.value(source);
            if (isPieceInSynthesisRange(runtime, piece)) {
                result.append(piece);
            }
        }
        return result;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::synthesisPiecesIn(const ClipRuntime *runtime, const QList<SynthesisPiece *> &pieces) const {
        QList<SynthesisPiece *> result;
        result.reserve(pieces.size());
        for (auto piece : pieces) {
            if (isPieceInSynthesisRange(runtime, piece)) {
                result.append(piece);
            }
        }
        return result;
    }

    void SynthesisProjectAddOn::deactivatePiece(SynthesisPiece *piece) {
        if (!piece) {
            return;
        }
        auto d = SynthesisPiecePrivate::get(piece);
        auto task = m_pieceTasks.take(piece);
        const bool changed = d->state != SynthesisPiece::Idle ||
                             !d->errorMessage.isEmpty() ||
                             !d->audioFilePath.isEmpty() ||
                             task;
        if (!changed) {
            return;
        }
        ++d->revision;
        const quint64 revision = d->revision;
        const bool running = task && task->state() == SynthesisTask::Running;
        if (task && task->state() == SynthesisTask::Queued) {
            discardTaskWritebacks(task);
            m_taskManager->cancel(task);
        }
        removeAudio(piece);
        d->errorMessage.clear();
        if (running) {
            connect(task, &SynthesisTask::finished, this, [this, piece = QPointer<SynthesisPiece>(piece), revision] {
                queueFinalizer([this, piece, revision] {
                    if (!piece || SynthesisPiecePrivate::get(piece)->revision != revision)
                        return;
                    auto runtime = runtimeForPiece(piece);
                    if (isPieceInSynthesisRange(runtime, piece))
                        return;
                    auto d = SynthesisPiecePrivate::get(piece);
                    d->state = SynthesisPiece::Idle;
                    Q_EMIT piece->stateChanged();
                    Q_EMIT m_context->pieceChanged(piece);
                    updateCounts();
                });
            });
        } else {
            d->state = SynthesisPiece::Idle;
        }
        Q_EMIT piece->stateChanged();
        Q_EMIT m_context->pieceChanged(piece);
    }

    bool SynthesisProjectAddOn::documentTransactionActive() const {
        if (m_transactionController && m_transactionController->isTransactionActive())
            return true;
        auto window = windowHandle() ? windowHandle()->cast<Core::ProjectWindowInterface>() : nullptr;
        auto document = window && window->projectDocumentContext()
                            ? window->projectDocumentContext()->document()
                            : nullptr;
        auto modelDocument = document && document->model() ? document->model()->document() : nullptr;
        auto transaction = modelDocument ? modelDocument->transaction() : nullptr;
        return transaction && transaction->state() == dini::TransactionState::Active;
    }

    void SynthesisProjectAddOn::schedulePendingWork() {
        if (m_pendingWorkScheduled)
            return;
        m_pendingWorkScheduled = true;
        QMetaObject::invokeMethod(this, [this] {
            m_pendingWorkScheduled = false;
            processPendingWork();
        }, Qt::QueuedConnection);
    }

    void SynthesisProjectAddOn::processPendingWork() {
        if (m_processingPendingWork || documentTransactionActive())
            return;
        m_processingPendingWork = true;
        while (!documentTransactionActive()) {
            bool processed{};
            if (m_commitPending) {
                m_commitPending = false;
                m_documentSyncPending = false;
                processCommittedChanges();
                processed = true;
            } else if (m_documentSyncPending) {
                m_documentSyncPending = false;
                synchronizeDocument();
                if (std::exchange(m_rollbackPending, false)) {
                    for (auto runtime : m_clips)
                        resetClipBaseline(runtime);
                }
                processed = true;
            }
            if (m_architectureSyncPending) {
                m_architectureSyncPending = false;
                const auto addedClips = synchronizeDocument();
                for (auto clip : addedClips)
                    resynthesizeClip(clip, SynthesisTaskType::Pronunciation, {});
                processed = true;
            }
            if (!m_pendingManualRequests.isEmpty()) {
                const auto requests = std::exchange(m_pendingManualRequests, QList<ManualRequest *>{});
                for (auto request : requests) {
                    processManualRequest(request);
                    delete request;
                }
                processed = true;
            }
            if (!m_pendingFinalizers.isEmpty()) {
                const auto finalizers = std::exchange(m_pendingFinalizers, QList<std::function<void()>>{});
                for (const auto &finalizer : finalizers)
                    finalizer();
                processed = true;
            }
            if (!m_pendingTaskWritebacks.isEmpty()) {
                const auto writebacks = std::exchange(m_pendingTaskWritebacks, QList<TaskWriteback *>{});
                for (auto writeback : writebacks) {
                    auto runtime = writeback ? m_clips.value(writeback->clipHandle) : nullptr;
                    m_taskWritebacks.remove(writeback);
                    processTaskWriteback(writeback);
                    delete writeback;
                    finalizeLanguageWave(runtime);
                }
                processed = true;
            }
            if (!processed)
                break;
        }
        m_processingPendingWork = false;
        if (!documentTransactionActive() &&
            (m_commitPending || m_documentSyncPending || m_architectureSyncPending ||
             !m_pendingManualRequests.isEmpty() || !m_pendingFinalizers.isEmpty() ||
             !m_pendingTaskWritebacks.isEmpty())) {
            schedulePendingWork();
        }
    }

    void SynthesisProjectAddOn::queueFinalizer(std::function<void()> finalizer) {
        m_pendingFinalizers.append(std::move(finalizer));
        schedulePendingWork();
    }

    void SynthesisProjectAddOn::queueTaskWriteback(TaskWriteback *writeback) {
        if (!writeback || writeback->queued)
            return;
        writeback->queued = true;
        if (writeback->task && writeback->task->state() == SynthesisTask::Succeeded) {
            const int expected = writeback->noteHandles.size();
            const auto result = writeback->task->result();
            if (writeback->type == SynthesisTaskType::Pronunciation &&
                result.pronunciations.size() != expected) {
                writeback->responseShapeError = tr("The pronunciation result does not match the requested score.");
            } else if (writeback->type == SynthesisTaskType::Phoneme &&
                       result.phonemes.size() != expected) {
                writeback->responseShapeError = tr("The phoneme result does not match the requested score.");
            } else if (writeback->type == SynthesisTaskType::Duration &&
                       result.phonemes.size() != expected) {
                writeback->responseShapeError = tr("The duration result does not match the requested piece score.");
            }
        }
        m_pendingTaskWritebacks.append(writeback);
        schedulePendingWork();
    }

    void SynthesisProjectAddOn::discardTaskWritebacks(SynthesisTask *task) {
        if (!task)
            return;
        for (auto writeback : m_taskWritebacks) {
            if (writeback && writeback->task == task)
                writeback->discarded = true;
        }
    }

    bool SynthesisProjectAddOn::hasUnprocessedWriteback(SynthesisTask *task) const {
        if (!task)
            return false;
        return std::ranges::any_of(m_taskWritebacks, [task](const TaskWriteback *writeback) {
            return writeback && !writeback->discarded && writeback->task == task;
        });
    }

    bool SynthesisProjectAddOn::validateTaskWriteback(const TaskWriteback *writeback) const {
        if (!writeback || !writeback->task || writeback->discarded || documentTransactionActive())
            return false;
        auto runtime = m_clips.value(writeback->clipHandle);
        if (!runtime || !runtime->clip || !runtime->divider || !isManagedClip(runtime->clip))
            return false;
        const auto context = buildSynthesisContext(runtime->clip);
        if (!context || context->architectureId != writeback->architecture.id())
            return false;
        if (writeback->scope == TaskWriteback::Language) {
            const auto current = buildLanguageRequest(
                runtime->clip, writeback->piecePosition, writeback->pieceLength,
                writeback->type, *context
            );
            return current.noteHandles == writeback->noteHandles &&
                   sameSynthesisInput(current.request, writeback->request) &&
                   !synthesisPiecesIn(runtime, piecesInRange(
                       runtime->clip, writeback->piecePosition, writeback->pieceLength
                   )).isEmpty();
        }
        auto piece = writeback->piece.data();
        if (!piece || piece->singingClip() != runtime->clip ||
            SynthesisPiecePrivate::get(piece)->revision != writeback->revision ||
            m_pieceTasks.value(piece) != writeback->task ||
            piece->position() != writeback->piecePosition ||
            piece->length() != writeback->pieceLength ||
            !isPieceInSynthesisRange(runtime, piece)) {
            return false;
        }
        const auto current = buildScore(
            windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip, piece,
            writeback->architecture, writeback->type == SynthesisTaskType::Audio,
            writeback->requestedParameters
        );
        if (!current.error.isEmpty() || current.noteHandles != writeback->noteHandles)
            return false;
        SynthesisTaskRequest request;
        request.type = writeback->type;
        request.context = *context;
        request.score = current.score;
        request.displayName = runtime->clip->name();
        return sameSynthesisInput(request, writeback->request);
    }

    void SynthesisProjectAddOn::processTaskWriteback(TaskWriteback *writeback) {
        if (!writeback)
            return;
        if (!validateTaskWriteback(writeback)) {
            if (writeback->discarded || !writeback->task ||
                !writeback->task->isFinished() || documentTransactionActive()) {
                return;
            }
            auto runtime = m_clips.value(writeback->clipHandle);
            if (!runtime || !runtime->clip || !runtime->divider || !isManagedClip(runtime->clip))
                return;
            QList<SynthesisPiece *> recoveryPieces = piecesInRange(
                runtime->clip, writeback->piecePosition, writeback->pieceLength
            );
            auto model = runtime->clip->model();
            for (const auto handle : writeback->noteHandles) {
                auto note = model ? model->find<dspx::Note>(handle) : nullptr;
                if (!note || note->noteSequence() != runtime->clip->notes())
                    continue;
                recoveryPieces.append(piecesInRange(
                    runtime->clip, note->position(), std::max(1, note->length())
                ));
            }
            QSet<SynthesisPiece *> uniqueRecovery(
                recoveryPieces.cbegin(), recoveryPieces.cend()
            );
            if (writeback->scope == TaskWriteback::Piece) {
                QList<SynthesisPiece *> orphanPieces;
                for (auto piece : synthesisPiecesIn(runtime, uniqueRecovery.values())) {
                    const auto owner = m_pieceTasks.value(piece);
                    if (owner && owner != writeback->task &&
                        (!owner->isFinished() || hasUnprocessedWriteback(owner))) {
                        continue;
                    }
                    orphanPieces.append(piece);
                }
                if (!orphanPieces.isEmpty()) {
                    invalidate(
                        runtime, writeback->type, orphanPieces, writeback->options,
                        writeback->requestedParameters
                    );
                }
                updateCounts();
                return;
            }
            for (auto piece : synthesisPiecesIn(runtime, uniqueRecovery.values())) {
                TaskWriteback *coverage{};
                for (auto candidate : m_taskWritebacks) {
                    if (!candidate || candidate->discarded || !candidate->task ||
                        candidate->scope != TaskWriteback::Language ||
                        candidate->clipHandle != runtime->clipHandle ||
                        !rangesOverlap(
                            piece->position(), piece->length(),
                            candidate->piecePosition, candidate->pieceLength
                        )) {
                        continue;
                    }
                    if (!coverage || candidate->type < coverage->type ||
                        (candidate->type == coverage->type &&
                         candidate->task->state() == SynthesisTask::Running)) {
                        coverage = candidate;
                    }
                }
                if (coverage) {
                    auto d = SynthesisPiecePrivate::get(piece);
                    d->state = coverage->task->state() == SynthesisTask::Running
                                   ? SynthesisPiece::Synthesizing
                                   : SynthesisPiece::Queued;
                    d->currentTaskType = coverage->type;
                    d->errorMessage.clear();
                    m_pieceTasks.insert(piece, coverage->task);
                    Q_EMIT piece->stateChanged();
                    Q_EMIT m_context->pieceChanged(piece);
                } else {
                    schedulePieceLanguage(
                        runtime, piece, writeback->type, writeback->options
                    );
                }
            }
            updateCounts();
            return;
        }
        auto runtime = m_clips.value(writeback->clipHandle);
        auto task = writeback->task.data();
        auto piece = writeback->piece.data();
        const auto fail = [this, runtime, piece, writeback](const QString &message) {
            if (writeback->scope == TaskWriteback::Language) {
                runtime->languageContinuations.append({
                    writeback->piecePosition,
                    writeback->pieceLength,
                    writeback->options,
                    writeback->architecture,
                    true,
                    false,
                    message,
                });
                const auto affectedPieces = synthesisPiecesIn(runtime, piecesInRange(
                    runtime->clip, writeback->piecePosition, writeback->pieceLength
                ));
                for (auto affected : affectedPieces) {
                    for (auto candidate : m_taskWritebacks) {
                        if (!candidate || candidate->discarded ||
                            candidate->scope != TaskWriteback::Language ||
                            candidate->clipHandle != runtime->clipHandle ||
                            !rangesOverlap(
                                affected->position(), affected->length(),
                                candidate->piecePosition, candidate->pieceLength
                            )) {
                            continue;
                        }
                        if (candidate != writeback) {
                            runtime->languageContinuations.append({
                                candidate->piecePosition,
                                candidate->pieceLength,
                                candidate->options,
                                candidate->architecture,
                                true,
                                false,
                                message,
                            });
                        }
                        candidate->discarded = true;
                        if (candidate->task && candidate->task->state() == SynthesisTask::Queued)
                            m_taskManager->cancel(candidate->task);
                    }
                    notifyFailure(affected, message);
                }
                return;
            }
            notifyFailure(piece, message);
        };
        if (task->state() != SynthesisTask::Succeeded) {
            fail(task->errorMessage().isEmpty() ? tr("Synthesis was canceled.") : task->errorMessage());
            return;
        }
        if (!writeback->responseShapeError.isEmpty()) {
            fail(writeback->responseShapeError);
            return;
        }

        auto model = runtime->clip->model();
        QList<dspx::Note *> notes;
        notes.reserve(writeback->noteHandles.size());
        for (const auto handle : writeback->noteHandles) {
            auto note = model->find<dspx::Note>(handle);
            if (!note || note->noteSequence() != runtime->clip->notes())
                return;
            notes.append(note);
        }
        const auto result = task->result();
        if (writeback->type == SynthesisTaskType::Pronunciation) {
            for (int index = 0; index < notes.size(); ++index)
                notes.at(index)->setOriginalPronunciation(result.pronunciations.at(index).pronunciation);
            scheduleLanguageRange(
                runtime, writeback->piecePosition, writeback->pieceLength,
                SynthesisTaskType::Phoneme, writeback->options
            );
            return;
        }
        if (writeback->type == SynthesisTaskType::Phoneme) {
            for (int index = 0; index < notes.size(); ++index)
                replaceOriginalPhonemes(notes.at(index), result.phonemes.at(index));
            runtime->languageContinuations.append({
                writeback->piecePosition,
                writeback->pieceLength,
                writeback->options,
                writeback->architecture,
                false,
                false,
                {},
            });
            const auto next = writeback->architecture.phonemeMode() == QStringLiteral("FULL")
                                  ? SynthesisTaskType::Duration
                                  : SynthesisTaskType::Parameter;
            for (auto affected : synthesisPiecesIn(runtime, piecesInRange(
                     runtime->clip, writeback->piecePosition, writeback->pieceLength
                 ))) {
                auto d = SynthesisPiecePrivate::get(affected);
                d->state = SynthesisPiece::Queued;
                d->currentTaskType = next;
                d->errorMessage.clear();
                if (m_pieceTasks.value(affected) == task)
                    m_pieceTasks.remove(affected);
                Q_EMIT affected->stateChanged();
                Q_EMIT m_context->pieceChanged(affected);
            }
            updateCounts();
            return;
        }
        if (writeback->type == SynthesisTaskType::Duration) {
            for (int index = 0; index < notes.size(); ++index)
                replaceOriginalPhonemes(notes.at(index), result.phonemes.at(index));
            schedulePieceStage(runtime, piece, SynthesisTaskType::Parameter, writeback->options);
            return;
        }
        if (writeback->type == SynthesisTaskType::Parameter) {
            if (!ensureParameterNodes(runtime, result.parameters.keys()) ||
                !validateTaskWriteback(writeback)) {
                return;
            }
            writeParameterOrigins(
                windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip,
                piece, result.parameters
            );
            schedulePieceStage(runtime, piece, SynthesisTaskType::Audio, writeback->options);
            return;
        }
        QString error;
        if (!installAudio(runtime, piece, result.audioFilePath, &error))
            notifyFailure(piece, error);
    }

    void SynthesisProjectAddOn::processManualRequest(ManualRequest *request) {
        if (!request)
            return;
        if (request->scope == ManualRequest::Project) {
            resynthesizeProject(request->fromType, request->options);
            return;
        }
        auto runtime = m_clips.value(request->clipHandle);
        if (!runtime || !runtime->clip)
            return;
        if (request->scope == ManualRequest::Clip) {
            resynthesizeClip(runtime->clip, request->fromType, request->options);
            return;
        }
        const auto affected = piecesInRange(runtime->clip, request->position, request->length);
        if (affected.isEmpty())
            return;
        for (auto piece : affected)
            resynthesizePiece(piece, request->fromType, request->options);
    }

    void SynthesisProjectAddOn::processCommittedChanges() {
        if (documentTransactionActive()) {
            m_commitPending = true;
            schedulePendingWork();
            return;
        }
        const auto addedClips = synchronizeDocument();
        const QSet<dspx::SingingClip *> addedClipSet(addedClips.cbegin(), addedClips.cend());
        for (auto runtime : m_clips) {
            if (!runtime || !runtime->clip || !runtime->divider || !runtime->watcher) {
                continue;
            }
            if (addedClipSet.contains(runtime->clip))
                continue;
            const bool rebound = std::exchange(runtime->rebound, false);
            const auto change = runtime->watcher->takeChanges();
            if (!rebound && change.isEmpty())
                continue;
            SynthesisTaskType from = SynthesisTaskType::Audio;
            if (rebound || change.contains(dspx::ClipChange::Sources) || change.contains(dspx::ClipChange::Lyric) ||
                change.contains(dspx::ClipChange::Score) || change.contains(dspx::ClipChange::ClipTiming)) {
                from = SynthesisTaskType::Pronunciation;
            } else if (change.contains(dspx::ClipChange::Pronunciation)) {
                from = SynthesisTaskType::Phoneme;
            } else if (change.contains(dspx::ClipChange::Note) || change.contains(dspx::ClipChange::Phoneme)) {
                from = SynthesisTaskType::Duration;
            } else if (change.contains(dspx::ClipChange::Vibrato) || change.contains(dspx::ClipChange::Parameter)) {
                from = SynthesisTaskType::Parameter;
            }
            const auto affectedPieces = [this, runtime, &change] {
                if (change.ranges().isEmpty()) {
                    return piecesForClip(runtime->clip);
                }
                QList<SynthesisPiece *> result;
                for (const auto &range : change.ranges()) {
                    result.append(piecesInRange(runtime->clip, range.position(), range.length()));
                }
                QSet<SynthesisPiece *> unique(result.cbegin(), result.cend());
                return unique.values();
            };

            const auto oldAffected = affectedPieces();
            QList<QPair<double, double>> invalidatedRanges;
            for (auto piece : oldAffected) {
                if (piece) {
                    invalidatedRanges.append({piece->position(), piece->length()});
                }
            }
            QList<QPair<double, double>> languageTopologyRanges;
            for (const auto &continuation : runtime->languageContinuations) {
                if (continuation.failed)
                    continue;
                for (auto piece : piecesInRange(
                         runtime->clip, continuation.position, continuation.length
                     )) {
                    if (piece)
                        languageTopologyRanges.append({piece->position(), piece->length()});
                }
            }
            for (auto writeback : m_taskWritebacks) {
                if (!writeback || writeback->discarded ||
                    writeback->scope != TaskWriteback::Language ||
                    writeback->clipHandle != runtime->clipHandle) {
                    continue;
                }
                for (auto piece : piecesInRange(
                         runtime->clip, writeback->piecePosition, writeback->pieceLength
                     )) {
                    if (piece)
                        languageTopologyRanges.append({piece->position(), piece->length()});
                }
            }
            std::optional<QStringList> requestedParameters;
            if (from == SynthesisTaskType::Parameter &&
                change.contains(dspx::ClipChange::Parameter) &&
                !change.contains(dspx::ClipChange::Vibrato) &&
                !change.parameterNames().isEmpty()) {
                const auto context = buildSynthesisContext(runtime->clip);
                const auto architecture = context ? architectureFor(*context) : ArchitectureMetadata{};
                if (!architecture.id().isEmpty()) {
                    requestedParameters = downstreamIndirectParameters(architecture, change.parameterNames());
                }
            }

            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
            auto affected = affectedPieces();
            for (const auto &[position, length] : invalidatedRanges)
                affected.append(piecesInRange(runtime->clip, position, length));
            QSet<SynthesisPiece *> uniqueAffected(affected.cbegin(), affected.cend());
            affected = uniqueAffected.values();
            for (auto piece : affected) {
                if (piece) {
                    invalidatedRanges.append({piece->position(), piece->length()});
                }
            }
            for (const auto &[position, length] : languageTopologyRanges)
                affected.append(piecesInRange(runtime->clip, position, length));
            uniqueAffected = QSet<SynthesisPiece *>(affected.cbegin(), affected.cend());
            affected = uniqueAffected.values();
            if (from <= SynthesisTaskType::Parameter) {
                clearParameterOrigins(runtime->clip, invalidatedRanges, requestedParameters);
            }
            invalidate(runtime, from, affected, {}, requestedParameters);
            QList<SynthesisPiece *> idlePieces;
            for (auto piece : synthesisPiecesForClip(runtime)) {
                if (piece && piece->state() == SynthesisPiece::Idle)
                    idlePieces.append(piece);
            }
            if (!idlePieces.isEmpty())
                invalidate(runtime, SynthesisTaskType::Pronunciation, idlePieces, {});
        }
        for (auto clip : addedClips)
            resynthesizeClip(clip, SynthesisTaskType::Pronunciation, {});
        updatePriorities();
    }

    void SynthesisProjectAddOn::finalizeLanguageWave(ClipRuntime *runtime) {
        if (!runtime || m_clips.value(runtime->clipHandle) != runtime ||
            !runtime->clip || !runtime->divider || documentTransactionActive()) {
            return;
        }
        const bool hasActiveLanguageTask = std::ranges::any_of(
            m_taskWritebacks, [runtime](const TaskWriteback *writeback) {
                return writeback && !writeback->discarded && writeback->task &&
                       writeback->scope == TaskWriteback::Language &&
                       writeback->clipHandle == runtime->clipHandle;
            }
        );
        if (hasActiveLanguageTask || runtime->languageContinuations.isEmpty()) {
            return;
        }

        const auto continuations = std::exchange(
            runtime->languageContinuations, QList<ClipRuntime::LanguageContinuation>{}
        );
        const bool hasSuccessfulPhoneme = std::ranges::any_of(
            continuations, [](const ClipRuntime::LanguageContinuation &continuation) {
                return !continuation.failed;
            }
        );
        QHash<SynthesisPiece *, QPair<double, double>> oldRanges;
        for (auto piece : runtime->pieces) {
            if (piece)
                oldRanges.insert(piece, {piece->position(), piece->length()});
        }
        if (hasSuccessfulPhoneme) {
            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
        }

        for (auto piece : synthesisPiecesForClip(runtime)) {
            const auto oldRange = oldRanges.constFind(piece);
            const bool topologyChanged = oldRange == oldRanges.cend() ||
                                         oldRange->first != piece->position() ||
                                         oldRange->second != piece->length();
            const ClipRuntime::LanguageContinuation *successful{};
            const ClipRuntime::LanguageContinuation *failed{};
            for (const auto &continuation : continuations) {
                if (!rangesOverlap(
                        piece->position(), piece->length(),
                        continuation.position, continuation.length
                    )) {
                    continue;
                }
                if (continuation.failed || continuation.canceled) {
                    failed = &continuation;
                } else {
                    successful = &continuation;
                }
            }
            if (topologyChanged && !successful && !failed) {
                double nearestDistance = std::numeric_limits<double>::max();
                const double pieceEnd = piece->position() + piece->length();
                const ClipRuntime::LanguageContinuation *nearest{};
                for (const auto &continuation : continuations) {
                    const double continuationEnd = continuation.position + continuation.length;
                    const double distance = pieceEnd <= continuation.position
                                                ? continuation.position - pieceEnd
                                            : continuationEnd <= piece->position()
                                                ? piece->position() - continuationEnd
                                                : 0.0;
                    if (distance < nearestDistance) {
                        nearestDistance = distance;
                        nearest = &continuation;
                    }
                }
                if (nearest && (nearest->failed || nearest->canceled)) {
                    failed = nearest;
                } else {
                    successful = nearest;
                }
            }
            if (failed) {
                m_pieceTasks.remove(piece);
                if (failed->errorMessage.isEmpty()) {
                    auto d = SynthesisPiecePrivate::get(piece);
                    ++d->revision;
                    d->state = SynthesisPiece::Stale;
                    d->errorMessage.clear();
                    removeAudio(piece);
                    Q_EMIT piece->stateChanged();
                    Q_EMIT m_context->pieceChanged(piece);
                } else {
                    notifyFailure(piece, failed->errorMessage);
                }
                continue;
            }
            if (!successful) {
                continue;
            }
            if (topologyChanged) {
                auto d = SynthesisPiecePrivate::get(piece);
                ++d->revision;
                d->state = SynthesisPiece::Stale;
                d->errorMessage.clear();
                removeAudio(piece);
            }
            const auto next = successful->architecture.phonemeMode() == QStringLiteral("FULL")
                                  ? SynthesisTaskType::Duration
                                  : SynthesisTaskType::Parameter;
            schedulePieceStage(runtime, piece, next, successful->options);
        }
        updatePriorities();
        updateCounts();
    }

    void SynthesisProjectAddOn::invalidate(ClipRuntime *runtime, SynthesisTaskType fromType, const QList<SynthesisPiece *> &affected, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters) {
        const auto synthesisAffected = synthesisPiecesIn(runtime, affected);
        if (synthesisAffected.isEmpty()) {
            updateCounts();
            return;
        }
        QList<SynthesisPiece *> toSchedule;
        for (auto piece : synthesisAffected) {
            if (!piece || !piece->singingClip()) {
                continue;
            }
            auto d = SynthesisPiecePrivate::get(piece);
            d->errorMessage.clear();
            if (fromType <= SynthesisTaskType::Audio) {
                removeAudio(piece);
                QString audioError;
                if (!prepareAudio(runtime, piece, &audioError)) {
                    notifyFailure(piece, audioError);
                    continue;
                }
            }
            const auto old = m_pieceTasks.value(piece);
            ClipRuntime::LanguageContinuation *completedLanguageCoverage{};
            ClipRuntime::LanguageContinuation *blockedLanguageCoverage{};
            if (fromType > SynthesisTaskType::Phoneme) {
                for (auto &continuation : runtime->languageContinuations) {
                    if (!rangesOverlap(
                            piece->position(), piece->length(),
                            continuation.position, continuation.length
                        )) {
                        continue;
                    }
                    if (continuation.failed || continuation.canceled) {
                        blockedLanguageCoverage = &continuation;
                    } else {
                        completedLanguageCoverage = &continuation;
                    }
                }
            }
            TaskWriteback *languageCoverage{};
            if (fromType > SynthesisTaskType::Phoneme) {
                for (auto candidate : m_taskWritebacks) {
                    if (!candidate || candidate->discarded || !candidate->task ||
                        candidate->scope != TaskWriteback::Language ||
                        candidate->clipHandle != runtime->clipHandle ||
                        candidate->type >= fromType ||
                        !rangesOverlap(
                            piece->position(), piece->length(),
                            candidate->piecePosition, candidate->pieceLength
                        )) {
                        continue;
                    }
                    if (!languageCoverage || candidate->type < languageCoverage->type ||
                        (candidate->type == languageCoverage->type &&
                         candidate->task->state() == SynthesisTask::Running)) {
                        languageCoverage = candidate;
                    }
                }
            }
            if (blockedLanguageCoverage) {
                if (old && old->state() == SynthesisTask::Queued) {
                    discardTaskWritebacks(old);
                    m_taskManager->cancel(old);
                }
                ++d->revision;
                m_pieceTasks.remove(piece);
                if (!blockedLanguageCoverage->errorMessage.isEmpty()) {
                    notifyFailure(piece, blockedLanguageCoverage->errorMessage);
                    continue;
                }
                removeAudio(piece);
                d->state = SynthesisPiece::Stale;
                d->errorMessage.clear();
                Q_EMIT piece->stateChanged();
                Q_EMIT m_context->pieceChanged(piece);
                continue;
            }
            if (languageCoverage || completedLanguageCoverage) {
                auto coverageTask = languageCoverage ? languageCoverage->task.data() : nullptr;
                if (old && old != coverageTask && old->state() == SynthesisTask::Queued) {
                    discardTaskWritebacks(old);
                    m_taskManager->cancel(old);
                }
                ++d->revision;
                d->state = coverageTask && coverageTask->state() == SynthesisTask::Running
                               ? SynthesisPiece::Synthesizing
                               : SynthesisPiece::Queued;
                d->currentTaskType = languageCoverage
                                         ? languageCoverage->type
                                         : completedLanguageCoverage->architecture.phonemeMode() == QStringLiteral("FULL")
                                             ? SynthesisTaskType::Duration
                                             : SynthesisTaskType::Parameter;
                if (coverageTask) {
                    m_pieceTasks.insert(piece, coverageTask);
                } else {
                    m_pieceTasks.remove(piece);
                }
                Q_EMIT piece->stateChanged();
                Q_EMIT m_context->pieceChanged(piece);
                continue;
            }
            const bool activeUpstreamTask = old && hasUnprocessedWriteback(old) &&
                                             old->type() < fromType;
            const bool coveredByCurrentPipeline = activeUpstreamTask;
            if (!coveredByCurrentPipeline) {
                ++d->revision;
                d->state = SynthesisPiece::Stale;
                if (old && old->state() == SynthesisTask::Queued) {
                    m_taskManager->cancel(old);
                }
                toSchedule.append(piece);
                Q_EMIT piece->stateChanged();
            }
            Q_EMIT m_context->pieceChanged(piece);
        }
        if (fromType <= SynthesisTaskType::Phoneme) {
            for (auto piece : toSchedule)
                schedulePieceLanguage(runtime, piece, fromType, options);
        } else {
            for (auto piece : toSchedule)
                schedulePieceStage(runtime, piece, fromType, options, requestedParameters);
        }
        updateCounts();
    }

    void SynthesisProjectAddOn::schedulePieceLanguage(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!runtime || !runtime->clip || !runtime->divider || !piece || piece->singingClip() != runtime->clip)
            return;
        if (!isPieceInSynthesisRange(runtime, piece)) {
            deactivatePiece(piece);
            return;
        }
        scheduleLanguageRange(runtime, piece->position(), piece->length(), fromType, options);
    }

    void SynthesisProjectAddOn::scheduleLanguageRange(ClipRuntime *runtime, double position, double length, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!runtime || !runtime->clip || !runtime->divider || length <= 0.0 || documentTransactionActive())
            return;
        if (!isManagedClip(runtime->clip)) {
            removeClip(runtime->clipHandle);
            return;
        }
        const auto affectedPieces = synthesisPiecesIn(
            runtime, piecesInRange(runtime->clip, position, length)
        );
        if (affectedPieces.isEmpty())
            return;
        const auto context = buildSynthesisContext(runtime->clip);
        if (!context) {
            for (auto piece : affectedPieces)
                notifyFailure(piece, tr("The clip has no valid singer source."));
            return;
        }
        const auto architecture = architectureFor(*context);
        if (architecture.id().isEmpty()) {
            for (auto piece : affectedPieces)
                notifyFailure(piece, tr("No healthy service provides the clip architecture."));
            return;
        }
        QList<ClipRuntime::LanguageContinuation> preservedContinuations;
        for (const auto &continuation : runtime->languageContinuations) {
            if (!rangesOverlap(position, length, continuation.position, continuation.length))
                continue;
            for (auto piece : synthesisPiecesIn(runtime, piecesInRange(
                     runtime->clip, continuation.position, continuation.length
                 ))) {
                if (rangesOverlap(position, length, piece->position(), piece->length()))
                    continue;
                auto preserved = continuation;
                preserved.position = piece->position();
                preserved.length = piece->length();
                preservedContinuations.append(preserved);
            }
        }
        runtime->languageContinuations.erase(
            std::remove_if(
                runtime->languageContinuations.begin(), runtime->languageContinuations.end(),
                [position, length](const ClipRuntime::LanguageContinuation &continuation) {
                    return rangesOverlap(position, length, continuation.position, continuation.length);
                }
            ),
            runtime->languageContinuations.end()
        );
        runtime->languageContinuations.append(preservedContinuations);
        QHash<SynthesisPiece *, TaskWriteback *> deferredLanguagePieces;
        for (auto oldWriteback : m_taskWritebacks) {
            if (!oldWriteback || oldWriteback->discarded ||
                oldWriteback->scope != TaskWriteback::Language ||
                oldWriteback->clipHandle != runtime->clipHandle ||
                !rangesOverlap(position, length, oldWriteback->piecePosition, oldWriteback->pieceLength)) {
                continue;
            }
            for (auto piece : synthesisPiecesIn(runtime, piecesInRange(
                     runtime->clip, oldWriteback->piecePosition, oldWriteback->pieceLength
                 ))) {
                if (rangesOverlap(position, length, piece->position(), piece->length()))
                    continue;
                const auto existing = deferredLanguagePieces.value(piece);
                if (!existing || oldWriteback->type < existing->type)
                    deferredLanguagePieces.insert(piece, oldWriteback);
            }
            oldWriteback->discarded = true;
            if (oldWriteback->task && oldWriteback->task->state() == SynthesisTask::Queued)
                m_taskManager->cancel(oldWriteback->task);
        }
        for (auto it = deferredLanguagePieces.cbegin(); it != deferredLanguagePieces.cend(); ++it) {
            auto piece = it.key();
            auto oldWriteback = it.value();
            if (!piece || !oldWriteback)
                continue;
            schedulePieceLanguage(
                runtime, piece, oldWriteback->type, oldWriteback->options
            );
        }
        if (fromType == SynthesisTaskType::Pronunciation &&
            architecture.pronunciationMode() == QStringLiteral("SKIP")) {
            fromType = SynthesisTaskType::Phoneme;
        }
        if (fromType == SynthesisTaskType::Phoneme &&
            architecture.phonemeMode() == QStringLiteral("SKIP")) {
            for (auto piece : affectedPieces)
                schedulePieceStage(runtime, piece, SynthesisTaskType::Parameter, options);
            return;
        }
        const auto built = buildLanguageRequest(runtime->clip, position, length, fromType, *context);
        if (built.noteHandles.isEmpty()) {
            for (auto piece : affectedPieces)
                notifyFailure(piece, tr("The synthesis task could not be queued."));
            return;
        }
        for (auto piece : affectedPieces) {
            const auto oldTask = m_pieceTasks.value(piece);
            if (oldTask && oldTask->state() == SynthesisTask::Queued) {
                discardTaskWritebacks(oldTask);
                m_taskManager->cancel(oldTask);
            }
            if (!m_audioBindings.contains(piece)) {
                QString audioError;
                if (!prepareAudio(runtime, piece, &audioError)) {
                    notifyFailure(piece, audioError);
                    return;
                }
            }
        }
        auto task = m_taskManager->enqueue(built.request, options);
        if (!task) {
            for (auto piece : affectedPieces)
                notifyFailure(piece, tr("The synthesis task could not be queued."));
            return;
        }
        for (auto piece : affectedPieces) {
            auto d = SynthesisPiecePrivate::get(piece);
            ++d->revision;
            d->state = SynthesisPiece::Queued;
            d->currentTaskType = fromType;
            d->errorMessage.clear();
            m_pieceTasks.insert(piece, task);
            Q_EMIT piece->stateChanged();
            Q_EMIT m_context->pieceChanged(piece);
        }
        auto writeback = new TaskWriteback;
        writeback->scope = TaskWriteback::Language;
        writeback->task = task;
        writeback->clipHandle = runtime->clipHandle;
        writeback->type = fromType;
        writeback->options = options;
        writeback->architecture = architecture;
        writeback->request = built.request;
        writeback->noteHandles = built.noteHandles;
        writeback->piecePosition = position;
        writeback->pieceLength = length;
        m_taskWritebacks.insert(writeback);
        connect(task, &SynthesisTask::stateChanged, this, [this, clipHandle = runtime->clipHandle, task, fromType, position, length] {
            if (task->state() != SynthesisTask::Running)
                return;
            queueFinalizer([this, clipHandle, task, fromType, position, length] {
                auto runtime = m_clips.value(clipHandle);
                if (!runtime || !runtime->clip || task->state() != SynthesisTask::Running ||
                    !hasUnprocessedWriteback(task)) {
                    return;
                }
                for (auto currentPiece : synthesisPiecesIn(
                         runtime, piecesInRange(runtime->clip, position, length)
                     )) {
                    auto d = SynthesisPiecePrivate::get(currentPiece);
                    d->state = SynthesisPiece::Synthesizing;
                    d->currentTaskType = fromType;
                    m_pieceTasks.insert(currentPiece, task);
                    Q_EMIT currentPiece->stateChanged();
                    Q_EMIT m_context->pieceChanged(currentPiece);
                }
                updateCounts();
            });
        });
        connect(task, &SynthesisTask::finished, this, [this, writeback] {
            queueTaskWriteback(writeback);
        });
        if (task->isFinished())
            queueTaskWriteback(writeback);
        updatePriorities();
        updateCounts();
    }

    void SynthesisProjectAddOn::schedulePieceStage(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTaskType type, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters) {
        if (!runtime || !runtime->clip || !piece || piece->singingClip() != runtime->clip)
            return;
        if (documentTransactionActive())
            return;
        if (!isManagedClip(runtime->clip)) {
            removeClip(runtime->clipHandle);
            return;
        }
        if (!isPieceInSynthesisRange(runtime, piece)) {
            deactivatePiece(piece);
            return;
        }
        QString audioError;
        if (!prepareAudio(runtime, piece, &audioError)) {
            notifyFailure(piece, audioError);
            return;
        }
        const auto context = buildSynthesisContext(runtime->clip);
        const auto architecture = context ? architectureFor(*context) : ArchitectureMetadata{};
        if (!context || architecture.id().isEmpty()) {
            notifyFailure(piece, tr("No healthy service can synthesize this piece."));
            return;
        }
        if (type == SynthesisTaskType::Duration &&
            architecture.phonemeMode() != QStringLiteral("FULL")) {
            schedulePieceStage(runtime, piece, SynthesisTaskType::Parameter, options);
            return;
        }
        const bool forAudio = type == SynthesisTaskType::Audio;
        auto built = buildScore(windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip, piece, architecture, forAudio, requestedParameters);
        if (!built.error.isEmpty()) {
            notifyFailure(piece, built.error);
            return;
        }
        if (type == SynthesisTaskType::Parameter && built.score.requestedParameters.isEmpty()) {
            schedulePieceStage(runtime, piece, SynthesisTaskType::Audio, options);
            return;
        }
        SynthesisTaskRequest request;
        request.type = type;
        request.context = *context;
        request.score = built.score;
        request.displayName = runtime->clip->name();
        const quint64 revision = SynthesisPiecePrivate::get(piece)->revision;
        auto task = m_taskManager->enqueue(request, options);
        if (!task) {
            notifyFailure(piece, tr("The synthesis task could not be queued."));
            return;
        }
        m_pieceTasks.insert(piece, task);
        bindPieceTask(runtime, piece, task, revision, options);
        auto writeback = new TaskWriteback;
        writeback->scope = TaskWriteback::Piece;
        writeback->task = task;
        writeback->clipHandle = runtime->clipHandle;
        writeback->piece = piece;
        writeback->type = type;
        writeback->options = options;
        writeback->architecture = architecture;
        writeback->request = request;
        writeback->noteHandles = built.noteHandles;
        writeback->requestedParameters = requestedParameters;
        writeback->revision = revision;
        writeback->piecePosition = piece->position();
        writeback->pieceLength = piece->length();
        m_taskWritebacks.insert(writeback);
        connect(task, &SynthesisTask::finished, this, [this, writeback] {
            queueTaskWriteback(writeback);
        });
        if (task->isFinished())
            queueTaskWriteback(writeback);
        updatePriorities();
    }

    void SynthesisProjectAddOn::bindPieceTask(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTask *task, quint64 revision, const SynthesisTaskOptions &) {
        auto d = SynthesisPiecePrivate::get(piece);
        d->state = SynthesisPiece::Queued;
        d->currentTaskType = task->type();
        d->errorMessage.clear();
        Q_EMIT piece->stateChanged();
        Q_EMIT m_context->pieceChanged(piece);
        connect(task, &SynthesisTask::stateChanged, this, [this, runtime, piece = QPointer<SynthesisPiece>(piece), task, revision] {
            if (task->state() != SynthesisTask::Running)
                return;
            queueFinalizer([this, runtime, piece, task, revision] {
                if (m_clips.value(runtime->clipHandle) != runtime || !piece ||
                    SynthesisPiecePrivate::get(piece)->revision != revision ||
                    task->state() != SynthesisTask::Running) {
                    return;
                }
                auto d = SynthesisPiecePrivate::get(piece);
                d->state = SynthesisPiece::Synthesizing;
                Q_EMIT piece->stateChanged();
                Q_EMIT m_context->pieceChanged(piece);
                updateCounts();
            });
        });
        updateCounts();
    }

    void SynthesisProjectAddOn::resynthesizeProject(SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (documentTransactionActive()) {
            auto request = new ManualRequest;
            request->scope = ManualRequest::Project;
            request->fromType = fromType;
            request->options = options;
            m_pendingManualRequests.append(request);
            schedulePendingWork();
            return;
        }
        synchronizeDocument();
        for (auto runtime : m_clips) {
            if (!runtime || !runtime->clip || !runtime->divider) {
                continue;
            }
            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
            invalidate(runtime, fromType, synthesisPiecesForClip(runtime), options);
        }
        updatePriorities();
    }

    void SynthesisProjectAddOn::resynthesizeClip(dspx::SingingClip *clip, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!clip)
            return;
        if (documentTransactionActive()) {
            auto request = new ManualRequest;
            request->scope = ManualRequest::Clip;
            request->clipHandle = clip->handle();
            request->fromType = fromType;
            request->options = options;
            m_pendingManualRequests.append(request);
            schedulePendingWork();
            return;
        }
        if (clip && !isManagedClip(clip)) {
            removeClip(clip->handle());
            return;
        }
        if (auto runtime = runtimeForClip(clip); runtime && runtime->divider) {
            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
            invalidate(runtime, fromType, synthesisPiecesForClip(runtime), options);
        }
        updatePriorities();
    }

    void SynthesisProjectAddOn::resynthesizePiece(SynthesisPiece *piece, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!piece)
            return;
        auto runtime = runtimeForPiece(piece);
        auto clip = piece->singingClip();
        if (documentTransactionActive()) {
            if (!runtime && !clip)
                return;
            auto request = new ManualRequest;
            request->scope = ManualRequest::Piece;
            request->clipHandle = runtime ? runtime->clipHandle : clip->handle();
            request->position = piece->position();
            request->length = piece->length();
            request->fromType = fromType;
            request->options = options;
            m_pendingManualRequests.append(request);
            schedulePendingWork();
            return;
        }
        if (!clip)
            return;
        if (!isManagedClip(clip)) {
            removeClip(clip->handle());
            return;
        }
        runtime = runtimeForClip(clip);
        if (!runtime || !runtime->clip || !runtime->divider)
            return;
        configureDivider(runtime->divider);
        runtime->divider->update();
        synchronizePieces(runtime);
        if (!runtime->pieces.values().contains(piece))
            return;
        if (!isPieceInSynthesisRange(runtime, piece))
            return;
        invalidate(runtime, fromType, {piece}, options);
        updatePriorities();
    }

    bool SynthesisProjectAddOn::cancelPiece(SynthesisPiece *piece) {
        if (!piece)
            return false;
        QSet<SynthesisTask *> tasks;
        bool canceledContinuation{};
        if (auto task = m_pieceTasks.value(piece);
            task && (!task->isFinished() || hasUnprocessedWriteback(task))) {
            tasks.insert(task);
        }
        const auto runtime = runtimeForPiece(piece);
        if (runtime) {
            for (auto &continuation : runtime->languageContinuations) {
                if (!rangesOverlap(
                        piece->position(), piece->length(),
                        continuation.position, continuation.length
                    )) {
                    continue;
                }
                continuation.canceled = true;
                continuation.errorMessage.clear();
                canceledContinuation = true;
            }
            for (auto writeback : m_taskWritebacks) {
                if (!writeback || writeback->discarded || !writeback->task ||
                    writeback->scope != TaskWriteback::Language ||
                    writeback->clipHandle != runtime->clipHandle ||
                    !rangesOverlap(piece->position(), piece->length(), writeback->piecePosition, writeback->pieceLength)) {
                    continue;
                }
                runtime->languageContinuations.append({
                    writeback->piecePosition,
                    writeback->pieceLength,
                    writeback->options,
                    writeback->architecture,
                    true,
                    false,
                    {},
                });
                writeback->discarded = true;
                tasks.insert(writeback->task);
            }
        }
        if (tasks.isEmpty() && !canceledContinuation)
            return false;
        for (auto writeback : m_taskWritebacks) {
            if (writeback && tasks.contains(writeback->task))
                writeback->discarded = true;
        }
        QSet<SynthesisPiece *> affectedPieces{piece};
        for (auto writeback : m_taskWritebacks) {
            if (!writeback || writeback->scope != TaskWriteback::Language ||
                !tasks.contains(writeback->task)) {
                continue;
            }
            auto taskRuntime = m_clips.value(writeback->clipHandle);
            if (!taskRuntime || !taskRuntime->clip)
                continue;
            const auto current = synthesisPiecesIn(taskRuntime, piecesInRange(
                taskRuntime->clip, writeback->piecePosition, writeback->pieceLength
            ));
            for (auto currentPiece : current)
                affectedPieces.insert(currentPiece);
        }
        for (auto affected : affectedPieces) {
            if (!affected)
                continue;
            auto d = SynthesisPiecePrivate::get(affected);
            ++d->revision;
            d->state = SynthesisPiece::Stale;
            d->errorMessage.clear();
            removeAudio(affected);
            if (tasks.contains(m_pieceTasks.value(affected)))
                m_pieceTasks.remove(affected);
            Q_EMIT affected->stateChanged();
            Q_EMIT m_context->pieceChanged(affected);
        }
        bool canceled = canceledContinuation;
        for (auto task : tasks) {
            if (task->isFinished()) {
                canceled = true;
            } else {
                canceled = m_taskManager->cancel(task) || canceled;
            }
        }
        finalizeLanguageWave(runtime);
        updateCounts();
        return canceled;
    }

    void SynthesisProjectAddOn::cancelAll() {
        for (auto piece : pieces())
            cancelPiece(piece);
        for (auto writeback : m_taskWritebacks) {
            if (!writeback || writeback->discarded || !writeback->task)
                continue;
            writeback->discarded = true;
            if (!writeback->task->isFinished())
                m_taskManager->cancel(writeback->task);
        }
        for (auto runtime : m_clips)
            finalizeLanguageWave(runtime);
        updateCounts();
    }

    void SynthesisProjectAddOn::updatePriorities() {
        if (!m_taskManager || documentTransactionActive())
            return;
        const int playhead = windowHandle()->cast<Core::ProjectWindowInterface>()->projectTimeline()->position();
        const auto priorityForRange = [playhead](double start, double end) {
            if (playhead >= start && playhead < end)
                return 1000000000;
            if (start >= playhead)
                return 500000000 - static_cast<int>(start - playhead);
            return 100000000 - static_cast<int>(playhead - end);
        };
        for (auto writeback : m_taskWritebacks) {
            if (!writeback || writeback->discarded || !writeback->task ||
                writeback->scope != TaskWriteback::Language ||
                writeback->task->state() != SynthesisTask::Queued) {
                continue;
            }
            auto runtime = m_clips.value(writeback->clipHandle);
            if (!runtime || !runtime->clip) {
                continue;
            }
            const double start = runtime->clip->start() + writeback->piecePosition;
            const double end = start + writeback->pieceLength;
            m_taskManager->setPriority(writeback->task, priorityForRange(start, end));
        }
        for (auto it = m_pieceTasks.cbegin(); it != m_pieceTasks.cend(); ++it) {
            auto piece = it.key();
            auto task = it.value();
            if (!piece || !task || task->state() != SynthesisTask::Queued)
                continue;
            if (task->type() <= SynthesisTaskType::Phoneme)
                continue;
            auto clip = piece->singingClip();
            if (!clip) {
                continue;
            }
            const double start = clip->start() + piece->position();
            const double end = start + piece->length();
            m_taskManager->setPriority(task, priorityForRange(start, end));
        }
    }

    void SynthesisProjectAddOn::updateCounts() {
        Q_EMIT m_context->pieceCountsChanged();
    }

    bool SynthesisProjectAddOn::ensureParameterNodes(ClipRuntime *runtime, const QStringList &parameterIds) {
        if (!runtime || !runtime->clip || parameterIds.isEmpty())
            return true;
        auto model = runtime->clip->model();
        auto document = model ? model->document() : nullptr;
        if (!document || document->transaction())
            return false;
        QStringList missing;
        for (const auto &id : parameterIds) {
            if (!id.isEmpty() && !runtime->clip->parameters()->containsKey(id))
                missing.append(id);
        }
        missing.removeDuplicates();
        if (missing.isEmpty())
            return true;

        auto transaction = document->engine()->beginTransaction({.undoable = false});
        document->setTransaction(&transaction);
        m_internalCommit = true;
        bool succeeded = true;
        try {
            for (const auto &id : missing) {
                auto parameter = model->createParameter();
                if (!runtime->clip->parameters()->insertItem(id, parameter)) {
                    model->destroyItem(parameter);
                    succeeded = false;
                    break;
                }
            }
            if (succeeded)
                transaction.commit();
            else
                transaction.rollback();
        } catch (...) {
            if (transaction.state() == dini::TransactionState::Active)
                transaction.rollback();
            succeeded = false;
        }
        document->setTransaction(nullptr);
        m_internalCommit = false;
        if (!succeeded) {
            qCWarning(lcSynthesisScheduler) << "Failed to create document parameter nodes for synthesis output";
            return false;
        }
        if (runtime->watcher) {
            runtime->watcher->takeChanges();
        }
        return true;
    }

    bool SynthesisProjectAddOn::prepareAudio(ClipRuntime *runtime, SynthesisPiece *piece, QString *errorMessage) {
        if (!runtime || !runtime->clip || !piece || piece->singingClip() != runtime->clip) {
            if (errorMessage) {
                *errorMessage = tr("The synthesis piece is no longer available.");
            }
            return false;
        }
        auto clipSequence = runtime->clip->clipSequence();
        auto track = clipSequence ? clipSequence->track() : nullptr;
        auto trackContext = track ? Audio::TrackAudioContext::of(track) : nullptr;
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto projectAudioContext = Audio::ProjectAudioContext::of(window);
        if (!trackContext || !projectAudioContext || !projectAudioContext->transport()) {
            if (errorMessage) {
                *errorMessage = tr("The project audio context is not available.");
            }
            return false;
        }

        if (runtime->audioSeries && runtime->audioTrackContext != trackContext) {
            detachAudioSeries(runtime);
        }
        if (!runtime->audioSeries) {
            runtime->audioSeries = new talcs::FutureAudioSourceClipSeries(this);
            runtime->audioSeries->setBufferingTarget(projectAudioContext->transport());
            runtime->audioSeries->setReadMode(talcs::FutureAudioSourceClipSeries::Notify);
            if (!trackContext->trackMixer()->addSource(runtime->audioSeries)) {
                delete runtime->audioSeries;
                runtime->audioSeries = nullptr;
                if (errorMessage) {
                    *errorMessage = tr("The synthesis buffer could not be inserted into the track mixer.");
                }
                return false;
            }
            runtime->audioTrackContext = trackContext;
        }

        const auto range = audioClipRange(window, trackContext, runtime->clip, piece);
        if (!range.isValid()) {
            if (errorMessage) {
                *errorMessage = tr("The synthesized piece is outside the visible clip range.");
            }
            return false;
        }
        if (auto binding = m_audioBindings.value(piece);
            binding && binding->series == runtime->audioSeries &&
            binding->range.position == range.position &&
            binding->range.sourceStart == range.sourceStart &&
            binding->range.length == range.length) {
            return true;
        }
        removeAudio(piece);

        auto promise = std::make_shared<QPromise<talcs::PositionableAudioSource *>>();
        const qint64 contentLength = std::max<qint64>(1, range.sourceStart + range.length);
        const int progressMaximum = static_cast<int>(std::min<qint64>(contentLength, std::numeric_limits<int>::max()));
        promise->setProgressRange(0, progressMaximum);
        promise->start();
        auto futureSource = new talcs::FutureAudioSource(promise->future(), {}, this);
        connect(futureSource, &talcs::FutureAudioSource::statusChanged, this, [this, piece = QPointer<SynthesisPiece>(piece)] {
            if (piece) {
                Q_EMIT m_context->pieceChanged(piece);
            }
        });
        const auto clipView = runtime->audioSeries->insertClip(futureSource, range.position, range.sourceStart, range.length);
        if (!clipView.isValid()) {
            futureSource->cancel();
            delete futureSource;
            promise->finish();
            if (errorMessage) {
                *errorMessage = tr("The synthesis buffer overlaps another piece in the same clip.");
            }
            return false;
        }
        m_audioBindings.insert(piece, new AudioBinding{
                                          runtime->audioSeries,
                                          clipView,
                                          futureSource,
                                          std::move(promise),
                                          nullptr,
                                          nullptr,
                                          range,
                                      });
        return true;
    }

    void SynthesisProjectAddOn::destroyAudioBinding(AudioBinding *binding) {
        if (!binding) {
            return;
        }
        if (binding->series && binding->clip.isValid()) {
            binding->series->removeClip(binding->clip);
        }
        if (binding->futureSource) {
            if (!binding->futureSource->source()) {
                binding->futureSource->cancel();
            }
            delete binding->futureSource;
        }
        if (binding->promise && !binding->promise->future().isFinished()) {
            binding->promise->finish();
        }
        if (binding->mixer && binding->source) {
            binding->mixer->removeSource(binding->source);
        }
        delete binding->mixer;
        delete binding->source;
        delete binding;
    }

    void SynthesisProjectAddOn::removeAudio(SynthesisPiece *piece) {
        if (!piece) {
            return;
        }
        auto binding = m_audioBindings.take(piece);
        destroyAudioBinding(binding);
        auto d = SynthesisPiecePrivate::get(piece);
        if (!d->audioFilePath.isEmpty()) {
            d->audioFilePath.clear();
            Q_EMIT piece->audioFileChanged();
        }
    }

    void SynthesisProjectAddOn::detachAudioSeries(ClipRuntime *runtime) {
        if (!runtime || !runtime->audioSeries) {
            return;
        }
        const auto pieces = m_audioBindings.keys();
        for (auto piece : pieces) {
            auto binding = m_audioBindings.value(piece);
            if (binding && binding->series == runtime->audioSeries) {
                removeAudio(piece);
            }
        }
        if (runtime->audioTrackContext) {
            runtime->audioTrackContext->trackMixer()->removeSource(runtime->audioSeries);
        }
        delete runtime->audioSeries;
        runtime->audioSeries = nullptr;
        runtime->audioTrackContext = nullptr;
    }

    bool SynthesisProjectAddOn::installAudio(ClipRuntime *runtime, SynthesisPiece *piece, const QString &filePath, QString *errorMessage) {
        if (!prepareAudio(runtime, piece, errorMessage)) {
            return false;
        }
        auto binding = m_audioBindings.value(piece);
        if (!binding || !binding->futureSource || !binding->promise ||
            binding->futureSource->source()) {
            if (errorMessage) {
                *errorMessage = tr("The synthesis buffer is no longer pending.");
            }
            return false;
        }
        auto io = Audio::GlobalAudioContext::formatManager()->getFormatLoad(filePath);
        if (!io) {
            if (errorMessage) {
                *errorMessage = tr("The synthesized audio file could not be opened.");
            }
            return false;
        }
        auto source = new talcs::AudioFormatInputSource(io, true);
        source->setStereoize(true);
        auto mixer = new talcs::PositionableMixerAudioSource;
        if (!mixer->addSource(source)) {
            delete mixer;
            delete source;
            if (errorMessage) {
                *errorMessage = tr("The synthesized audio source could not be prepared.");
            }
            return false;
        }
        mixer->setGain(static_cast<float>(runtime->clip->gain()));
        mixer->setPan(static_cast<float>(runtime->clip->pan()));
        mixer->setSilentFlags(runtime->clip->mute() ? -1 : 0);
        connect(runtime->clip, &dspx::Clip::gainChanged, mixer, &talcs::PositionableMixerAudioSource::setGain);
        connect(runtime->clip, &dspx::Clip::panChanged, mixer, &talcs::PositionableMixerAudioSource::setPan);
        connect(runtime->clip, &dspx::Clip::muteChanged, mixer, [mixer](bool mute) {
            mixer->setSilentFlags(mute ? -1 : 0);
        });
        binding->mixer = mixer;
        binding->source = source;
        auto d = SynthesisPiecePrivate::get(piece);
        d->audioFilePath = filePath;
        d->errorMessage.clear();
        Q_EMIT piece->audioFileChanged();
        const QPointer<talcs::FutureAudioSource> futureSource = binding->futureSource;
        connect(futureSource.data(), &talcs::FutureAudioSource::statusChanged, this, [this, piece = QPointer<SynthesisPiece>(piece), futureSource](talcs::FutureAudioSource::Status status) {
            if (status != talcs::FutureAudioSource::Ready)
                return;
            queueFinalizer([this, piece, futureSource] {
                if (!futureSource || !futureSource->source() || !piece)
                    return;
                const auto binding = m_audioBindings.value(piece);
                if (!binding || binding->futureSource != futureSource.data())
                    return;
                auto d = SynthesisPiecePrivate::get(piece);
                d->state = SynthesisPiece::Ready;
                d->errorMessage.clear();
                Q_EMIT piece->stateChanged();
                Q_EMIT m_context->pieceChanged(piece);
                updateCounts();
            });
        });
        binding->promise->setProgressValue(binding->promise->future().progressMaximum());
        binding->promise->addResult(mixer);
        binding->promise->finish();
        return true;
    }

    bool SynthesisProjectAddOn::waitForAudioSynthesis(QString *errorMessage) {
        if (documentTransactionActive()) {
            if (errorMessage) {
                *errorMessage = tr("Audio export cannot start while an edit is in progress. Finish or cancel the current edit and try again.");
            }
            return false;
        }
        processPendingWork();
        if (documentTransactionActive()) {
            if (errorMessage) {
                *errorMessage = tr("Audio export cannot start while an edit is in progress. Finish or cancel the current edit and try again.");
            }
            return false;
        }
        synchronizeDocument();

        QEventLoop eventLoop;
        connect(m_context, &ProjectSynthesisContext::pieceChanged, &eventLoop, &QEventLoop::quit);
        connect(m_context, &ProjectSynthesisContext::piecesChanged, &eventLoop, &QEventLoop::quit);
        connect(m_taskManager, &SynthesisTaskManager::taskChanged, &eventLoop, &QEventLoop::quit);
        if (m_transactionController) {
            connect(m_transactionController.data(), &Core::TransactionController::transactionActiveChanged,
                    &eventLoop, &QEventLoop::quit);
        }

        QSet<dspx::Handle> requestedClips;
        while (true) {
            if (documentTransactionActive()) {
                if (errorMessage) {
                    *errorMessage = tr("Audio export cannot continue while an edit is in progress. Finish or cancel the current edit and try again.");
                }
                return false;
            }
            bool waiting{};
            QSet<dspx::Handle> clipsToStart;
            for (auto runtime : m_clips) {
                for (auto piece : synthesisPiecesForClip(runtime)) {
                    if (!piece) {
                        continue;
                    }
                    auto clip = piece->singingClip();
                    if (!clip) {
                        continue;
                    }
                    switch (piece->state()) {
                        case SynthesisPiece::Idle:
                        case SynthesisPiece::Stale:
                            if (requestedClips.contains(runtime->clipHandle)) {
                                if (errorMessage) {
                                    *errorMessage = tr("Audio synthesis could not be started for clip \"%1\".").arg(clip->name());
                                }
                                return false;
                            }
                            clipsToStart.insert(runtime->clipHandle);
                            break;
                        case SynthesisPiece::Queued:
                        case SynthesisPiece::Synthesizing:
                            waiting = true;
                            break;
                        case SynthesisPiece::Ready: {
                            const auto binding = m_audioBindings.value(piece);
                            if (!binding || !binding->futureSource || piece->audioFilePath().isEmpty()) {
                                if (errorMessage) {
                                    *errorMessage = tr("Audio export was stopped because clip \"%1\" has no completed synthesized audio.").arg(clip->name());
                                }
                                return false;
                            }
                            if (binding->futureSource->status() == talcs::FutureAudioSource::Cancelled) {
                                if (errorMessage) {
                                    *errorMessage = tr("Audio export was stopped because synthesized audio for clip \"%1\" was canceled.").arg(clip->name());
                                }
                                return false;
                            }
                            if (!binding->futureSource->source()) {
                                waiting = true;
                            }
                            break;
                        }
                        case SynthesisPiece::Failed:
                            if (errorMessage) {
                                const auto detail = piece->errorMessage().isEmpty()
                                                        ? tr("Unknown synthesis error.")
                                                        : piece->errorMessage();
                                *errorMessage = tr("Audio export was stopped because synthesis failed for clip \"%1\": %2")
                                                    .arg(clip->name(), detail);
                            }
                            return false;
                    }
                }
            }

            if (!clipsToStart.isEmpty()) {
                for (const auto clipHandle : clipsToStart) {
                    requestedClips.insert(clipHandle);
                    auto runtime = m_clips.value(clipHandle);
                    if (runtime && runtime->clip)
                        resynthesizeClip(runtime->clip, SynthesisTaskType::Pronunciation, {});
                }
                continue;
            }
            if (!waiting) {
                return true;
            }
            eventLoop.exec();
        }
    }

    void SynthesisProjectAddOn::notifyFailure(SynthesisPiece *piece, const QString &message) {
        if (!piece)
            return;
        removeAudio(piece);
        auto d = SynthesisPiecePrivate::get(piece);
        d->state = SynthesisPiece::Failed;
        d->errorMessage = message;
        Q_EMIT piece->stateChanged();
        Q_EMIT m_context->pieceChanged(piece);
        const auto now = QDateTime::currentDateTimeUtc();
        const auto previous = m_lastNotifications.value(message);
        if (!previous.isValid() || previous.msecsTo(now) > 2000) {
            m_lastNotifications.insert(message, now);
            windowHandle()->cast<Core::ProjectWindowInterface>()->sendNotification(
                SVS::SVSCraft::Critical, tr("Synthesis failed"), message,
                Core::ProjectWindowInterface::AutoHide
            );
        }
        updateCounts();
    }

}

#include "moc_SynthesisProjectAddOn.cpp"
