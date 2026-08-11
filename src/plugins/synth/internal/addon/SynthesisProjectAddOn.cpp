#include "SynthesisProjectAddOn.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPointer>
#include <QQmlComponent>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <opendspx/anchornode.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/SVSCraftNamespace.h>

#include <TalcsCore/AudioSourceClipSeries.h>
#include <TalcsCore/PositionableMixerAudioSource.h>
#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/AudioFormatInputSource.h>
#include <TalcsFormat/FormatManager.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/GlobalAudioContext.h>
#include <audio/ProjectAudioContext.h>
#include <audio/TrackAudioContext.h>
#include <dini/engine.h>
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
                                                      QStringLiteral("piecePaddingAdditionalMs"), 100.0
            )
                                              .toDouble());
            divider->setPaddingGap(settings->value(QStringLiteral("piecePaddingGapMs"), 1000.0).toDouble());
            divider->setRestLyrics(settings->value(
                                               QStringLiteral("pieceRestLyrics"),
                                               QStringList{QStringLiteral("AP"), QStringLiteral("SP")}
            )
                                       .toStringList());
            settings->endGroup();
        }

        struct FlattenedSinger {
            SynthesisSinger singer;
            int rootIndex{};
            double nestedWeight{};
        };

        struct BuiltScore {
            SynthesisScore score;
            QList<QPointer<dspx::Note>> notes;
            QString error;
        };

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

        std::optional<double> freeValue(const dspx::FreeValueDataArray *array, double tick) {
            if (!array || tick < 0.0) {
                return std::nullopt;
            }
            const double index = tick / dspx::FreeValueDataArray::step();
            const int leftIndex = static_cast<int>(std::floor(index));
            const auto values = array->slice(leftIndex, 2);
            if (values.isEmpty() || !values.first().isValid() || values.first().isNull()) {
                return std::nullopt;
            }
            const double left = values.first().toDouble();
            const double fraction = index - leftIndex;
            if (qFuzzyIsNull(fraction)) {
                return left;
            }
            if (values.size() < 2 || !values.at(1).isValid() || values.at(1).isNull()) {
                return std::nullopt;
            }
            return left + (values.at(1).toDouble() - left) * fraction;
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
            explicit ParameterValueEvaluator(dspx::Parameter *parameter)
                : m_parameter(parameter),
                  m_editedAnchors(buildAnchorCurve(parameter ? parameter->anchorEdited() : nullptr)),
                  m_transformAnchors(buildAnchorCurve(parameter ? parameter->anchorTransform() : nullptr)) {
            }

            double evaluate(double tick, double fallback) const {
                if (!m_parameter) {
                    return fallback;
                }
                auto base = anchorValue(m_editedAnchors, tick);
                if (!base) {
                    base = freeValue(m_parameter->freeEdited(), tick);
                }
                if (!base) {
                    base = freeValue(m_parameter->original(), tick);
                }
                if (!base) {
                    base = fallback;
                }
                auto transform = anchorValue(m_transformAnchors, tick);
                if (!transform) {
                    transform = freeValue(m_parameter->freeTransform(), tick);
                }
                return *base * (transform ? *transform / 1000.0 : 1.0);
            }

        private:
            dspx::Parameter *m_parameter{};
            std::vector<AnchorCurveSegment> m_editedAnchors;
            std::vector<AnchorCurveSegment> m_transformAnchors;
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
                    result.error = SynthesisProjectAddOn::tr("Overlapping notes cannot be represented by the selected DSSP score model.");
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
                result.notes.append(note);
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
            for (const auto &id : parameterIds) {
                SynthesisParameter parameter;
                parameter.sampleRate = parameterSampleRate;
                const auto configuration = parameterConfiguration(architecture.id(), id);
                const int fallback = configuration.id().isEmpty() ? 0 : configuration.defaultValue();
                const ParameterValueEvaluator evaluator(clip->parameters()->item(id));
                for (int frame = 0; frame < parameterFrames; ++frame) {
                    const double seconds = pieceStartSeconds + frame / parameter.sampleRate;
                    const int relativeTick = timeline->create(seconds * 1000.0).totalTick() - clip->start();
                    const int defaultValue = id == QStringLiteral("pitch")
                                                 ? pitchAt(clip, relativeTick)
                                                 : fallback;
                    parameter.values.append(normalizedParameterValue(evaluator, configuration, relativeTick, defaultValue));
                }
                result.score.parameters.insert(id, parameter);
            }
            return result;
        }

        SynthesisTaskRequest languageRequest(dspx::SingingClip *clip, SynthesisTaskType type, const SynthesisContext &context) {
            SynthesisTaskRequest request;
            request.type = type;
            request.context = context;
            request.displayName = clip->name();
            for (auto note : clip->notes()->asRange()) {
                if (type == SynthesisTaskType::Pronunciation) {
                    request.lyricNotes.append({note->lyric(), note->language()});
                } else {
                    request.pronunciationNotes.append({effectivePronunciation(note), note->language()});
                }
            }
            return request;
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

    }

    struct SynthesisProjectAddOn::ClipRuntime {
        QPointer<dspx::SingingClip> clip;
        dspx::PieceDivider *divider{};
        dspx::ClipWatcher *watcher{};
        QHash<dspx::Piece *, SynthesisPiece *> pieces;
        QPointer<SynthesisTask> languageTask;
        quint64 revision{};
    };

    struct SynthesisProjectAddOn::AudioBinding {
        QPointer<Audio::TrackAudioContext> trackContext;
        talcs::AudioSourceClipSeries *series{};
        talcs::AudioSourceClipSeries::ClipView clip;
        talcs::PositionableMixerAudioSource *mixer{};
        talcs::AudioFormatInputSource *source{};
    };

    SynthesisProjectAddOn::SynthesisProjectAddOn(QObject *parent)
        : Core::WindowInterfaceAddOn(parent) {
    }

    SynthesisProjectAddOn::~SynthesisProjectAddOn() {
        m_subscription.disconnect();
        cancelAll();
        const auto audioPieces = m_audioBindings.keys();
        for (auto piece : audioPieces)
            removeAudio(piece);
        qDeleteAll(m_clips);
        qDeleteAll(m_retiredClips);
    }

    void SynthesisProjectAddOn::initialize() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        m_taskManager = SynthInterface::instance()->taskManager();
        m_context = new ProjectSynthesisContext(window, this);
        ProjectSynthesisContextPrivate::get(m_context)->controller = this;
        window->addObject(m_context);

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

        auto document = window->projectDocumentContext()->document();
        auto engine = document->model()->document()->engine();
        m_subscription = engine->subscribe([this](const dini::EngineEvent &event) {
            if (event.kind != dini::EventKind::AfterCommit || m_internalCommit || m_commitPending) {
                return;
            }
            m_commitPending = true;
            QMetaObject::invokeMethod(this, [this] {
                m_commitPending = false;
                processCommittedChanges(); }, Qt::QueuedConnection);
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
        for (auto runtime : m_clips)
            result.append(runtime->pieces.values());
        std::sort(result.begin(), result.end(), [](SynthesisPiece *left, SynthesisPiece *right) {
            const double leftStart = left->singingClip()->start() + left->position();
            const double rightStart = right->singingClip()->start() + right->position();
            return leftStart < rightStart;
        });
        return result;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::piecesForClip(dspx::SingingClip *clip) const {
        const auto runtime = m_clips.value(clip);
        if (!runtime)
            return {};
        QList<SynthesisPiece *> result;
        for (auto piece : runtime->divider->pieces()) {
            if (runtime->pieces.contains(piece))
                result.append(runtime->pieces.value(piece));
        }
        return result;
    }

    QList<SynthesisPiece *> SynthesisProjectAddOn::piecesInRange(dspx::SingingClip *clip, double position, double length) const {
        const auto runtime = m_clips.value(clip);
        if (!runtime)
            return {};
        QList<SynthesisPiece *> result;
        for (auto piece : runtime->divider->slice(position, length)) {
            if (runtime->pieces.contains(piece))
                result.append(runtime->pieces.value(piece));
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

    void SynthesisProjectAddOn::synchronizeDocument() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto model = window->projectDocumentContext()->document()->model();
        QSet<dspx::SingingClip *> live;
        for (auto track : model->tracks()->items()) {
            for (auto clip : track->clips()->asRange()) {
                if (clip->type() == dspx::Clip::Singing) {
                    auto singing = static_cast<dspx::SingingClip *>(clip);
                    live.insert(singing);
                    if (!m_clips.contains(singing))
                        addClip(singing);
                }
            }
        }
        for (auto clip : m_clips.keys()) {
            if (!live.contains(clip))
                removeClip(clip);
        }
    }

    void SynthesisProjectAddOn::addClip(dspx::SingingClip *clip) {
        auto runtime = new ClipRuntime;
        runtime->clip = clip;
        runtime->divider = new dspx::PieceDivider(this);
        runtime->watcher = new dspx::ClipWatcher(this);
        configureDivider(runtime->divider);
        runtime->divider->setSingingClip(clip);
        runtime->watcher->setSingingClip(clip);
        runtime->divider->update();
        m_clips.insert(clip, runtime);
        synchronizePieces(runtime);
    }

    void SynthesisProjectAddOn::removeClip(dspx::SingingClip *clip) {
        auto runtime = m_clips.take(clip);
        if (!runtime)
            return;
        ++runtime->revision;
        if (runtime->languageTask && runtime->languageTask->state() == SynthesisTask::Queued)
            m_taskManager->cancel(runtime->languageTask);
        runtime->clip = nullptr;
        for (auto piece : runtime->pieces) {
            auto d = SynthesisPiecePrivate::get(piece);
            ++d->revision;
            if (auto task = m_pieceTasks.value(piece); task && task->state() == SynthesisTask::Queued)
                m_taskManager->cancel(task);
            removeAudio(piece);
            piece->deleteLater();
        }
        runtime->pieces.clear();
        delete runtime->divider;
        delete runtime->watcher;
        runtime->divider = nullptr;
        runtime->watcher = nullptr;
        m_retiredClips.append(runtime);
        Q_EMIT m_context->piecesChanged();
        updateCounts();
    }

    void SynthesisProjectAddOn::synchronizePieces(ClipRuntime *runtime) {
        QList<dspx::Piece *> visiblePieces;
        const double visibleStart = runtime->clip->clipStart();
        const double visibleEnd = visibleStart + runtime->clip->clipLength();
        for (auto source : runtime->divider->pieces()) {
            if (source->position() + source->length() > visibleStart &&
                source->position() < visibleEnd) {
                visiblePieces.append(source);
            }
        }
        QSet<dspx::Piece *> live(visiblePieces.cbegin(), visiblePieces.cend());
        bool changed{};
        for (auto source : visiblePieces) {
            auto piece = runtime->pieces.value(source);
            if (!piece) {
                piece = new SynthesisPiece(runtime->clip, m_context);
                runtime->pieces.insert(source, piece);
                connect(piece, &QObject::destroyed, this, [this, piece] {
                    m_pieceTasks.remove(piece);
                    m_audioBindings.remove(piece);
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
            auto d = SynthesisPiecePrivate::get(piece);
            ++d->revision;
            if (auto task = m_pieceTasks.value(piece); task && task->state() == SynthesisTask::Queued)
                m_taskManager->cancel(task);
            removeAudio(piece);
            piece->deleteLater();
            changed = true;
        }
        if (changed)
            Q_EMIT m_context->piecesChanged();
        updateCounts();
    }

    void SynthesisProjectAddOn::processCommittedChanges() {
        synchronizeDocument();
        for (auto runtime : m_clips) {
            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
            const auto change = runtime->watcher->takeChanges();
            if (change.isEmpty())
                continue;
            SynthesisTaskType from = SynthesisTaskType::Audio;
            if (change.contains(dspx::ClipChange::Sources) || change.contains(dspx::ClipChange::Lyric) ||
                change.contains(dspx::ClipChange::Score)) {
                from = SynthesisTaskType::Pronunciation;
            } else if (change.contains(dspx::ClipChange::Pronunciation)) {
                from = SynthesisTaskType::Phoneme;
            } else if (change.contains(dspx::ClipChange::Note) || change.contains(dspx::ClipChange::Phoneme)) {
                from = SynthesisTaskType::Duration;
            } else if (change.contains(dspx::ClipChange::Vibrato) || change.contains(dspx::ClipChange::Parameter)) {
                from = SynthesisTaskType::Parameter;
            }
            QList<SynthesisPiece *> affected;
            if (from <= SynthesisTaskType::Phoneme || change.ranges().isEmpty()) {
                affected = piecesForClip(runtime->clip);
            } else {
                for (const auto &range : change.ranges()) {
                    affected.append(piecesInRange(runtime->clip, range.position(), range.length()));
                }
                QSet<SynthesisPiece *> uniqueAffected(affected.cbegin(), affected.cend());
                affected = uniqueAffected.values();
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
            invalidate(runtime, from, affected, {}, requestedParameters);
        }
        updatePriorities();
    }

    void SynthesisProjectAddOn::invalidate(ClipRuntime *runtime, SynthesisTaskType fromType, const QList<SynthesisPiece *> &affected, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters) {
        const bool languagePipelineActive = runtime->languageTask &&
                                            !runtime->languageTask->isFinished();
        if (fromType <= SynthesisTaskType::Phoneme) {
            ++runtime->revision;
            if (runtime->languageTask &&
                runtime->languageTask->state() == SynthesisTask::Queued) {
                m_taskManager->cancel(runtime->languageTask);
            }
        }
        QList<SynthesisPiece *> toSchedule;
        for (auto piece : affected) {
            auto d = SynthesisPiecePrivate::get(piece);
            d->errorMessage.clear();
            if (fromType <= SynthesisTaskType::Audio)
                removeAudio(piece);
            const auto old = m_pieceTasks.value(piece);
            const bool activeUpstreamTask = old && !old->isFinished() &&
                                            old->type() < fromType;
            const bool coveredByCurrentPipeline = fromType > SynthesisTaskType::Phoneme &&
                                                  (languagePipelineActive || activeUpstreamTask);
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
            scheduleClipLanguage(runtime, fromType, options);
        } else if (!languagePipelineActive) {
            for (auto piece : toSchedule)
                schedulePieceStage(runtime, piece, fromType, options, requestedParameters);
        }
        updateCounts();
    }

    void SynthesisProjectAddOn::scheduleClipLanguage(ClipRuntime *runtime, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!runtime->clip)
            return;
        const auto context = buildSynthesisContext(runtime->clip);
        if (!context) {
            for (auto piece : runtime->pieces)
                notifyFailure(piece, tr("The clip has no valid singer source."));
            return;
        }
        const auto architecture = architectureFor(*context);
        if (architecture.id().isEmpty()) {
            for (auto piece : runtime->pieces)
                notifyFailure(piece, tr("No healthy service provides the clip architecture."));
            return;
        }
        if (fromType == SynthesisTaskType::Pronunciation &&
            architecture.pronunciationMode() == QStringLiteral("SKIP")) {
            fromType = SynthesisTaskType::Phoneme;
        }
        if (fromType == SynthesisTaskType::Phoneme &&
            architecture.phonemeMode() == QStringLiteral("SKIP")) {
            runtime->divider->update();
            synchronizePieces(runtime);
            for (auto piece : piecesForClip(runtime->clip))
                schedulePieceStage(runtime, piece, SynthesisTaskType::Parameter, options);
            return;
        }
        const quint64 revision = runtime->revision;
        auto task = m_taskManager->enqueue(languageRequest(runtime->clip, fromType, *context), options);
        if (!task)
            return;
        runtime->languageTask = task;
        for (auto piece : runtime->pieces) {
            auto d = SynthesisPiecePrivate::get(piece);
            d->state = SynthesisPiece::Queued;
            d->currentTaskType = fromType;
            Q_EMIT piece->stateChanged();
        }
        connect(task, &SynthesisTask::stateChanged, this, [this, runtime, task] {
            if (task->state() != SynthesisTask::Running)
                return;
            for (auto piece : runtime->pieces) {
                auto d = SynthesisPiecePrivate::get(piece);
                d->state = SynthesisPiece::Synthesizing;
                Q_EMIT piece->stateChanged();
            }
            updateCounts();
        });
        connect(task, &SynthesisTask::finished, this, [this, runtime, task, revision, fromType, options, architecture] {
            if (!runtime->clip || runtime->revision != revision)
                return;
            if (task->state() != SynthesisTask::Succeeded) {
                for (auto piece : runtime->pieces)
                    notifyFailure(piece, task->errorMessage().isEmpty() ? tr("Synthesis was canceled.") : task->errorMessage());
                return;
            }
            const auto notes = runtime->clip->notes()->asRange();
            QList<dspx::Note *> noteList;
            for (auto note : notes)
                noteList.append(note);
            if (fromType == SynthesisTaskType::Pronunciation) {
                if (task->result().pronunciations.size() != noteList.size()) {
                    for (auto piece : runtime->pieces)
                        notifyFailure(piece, tr("The pronunciation result does not match the current score."));
                    return;
                }
                for (int index = 0; index < noteList.size(); ++index)
                    noteList.at(index)->setOriginalPronunciation(task->result().pronunciations.at(index).pronunciation);
                scheduleClipLanguage(runtime, SynthesisTaskType::Phoneme, options);
                return;
            }
            if (task->result().phonemes.size() != noteList.size()) {
                for (auto piece : runtime->pieces)
                    notifyFailure(piece, tr("The phoneme result does not match the current score."));
                return;
            }
            for (int index = 0; index < noteList.size(); ++index)
                replaceOriginalPhonemes(noteList.at(index), task->result().phonemes.at(index));
            runtime->divider->update();
            synchronizePieces(runtime);
            const auto next = architecture.phonemeMode() == QStringLiteral("FULL")
                                  ? SynthesisTaskType::Duration
                                  : SynthesisTaskType::Parameter;
            for (auto piece : piecesForClip(runtime->clip))
                schedulePieceStage(runtime, piece, next, options);
        });
        updatePriorities();
        updateCounts();
    }

    void SynthesisProjectAddOn::schedulePieceStage(ClipRuntime *runtime, SynthesisPiece *piece, SynthesisTaskType type, const SynthesisTaskOptions &options, const std::optional<QStringList> &requestedParameters) {
        if (!runtime || !runtime->clip || !piece)
            return;
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
        if (!task)
            return;
        m_pieceTasks.insert(piece, task);
        bindPieceTask(runtime, piece, task, revision, options);
        connect(task, &SynthesisTask::finished, this, [this, runtime, piece = QPointer<SynthesisPiece>(piece), task, revision, type, options, notes = built.notes] {
            if (!piece || !runtime->clip || SynthesisPiecePrivate::get(piece)->revision != revision)
                return;
            if (task->state() != SynthesisTask::Succeeded) {
                notifyFailure(piece, task->errorMessage().isEmpty() ? tr("Synthesis was canceled.") : task->errorMessage());
                return;
            }
            if (type == SynthesisTaskType::Duration) {
                if (task->result().phonemes.size() != notes.size()) {
                    notifyFailure(piece, tr("The duration result does not match the current piece score."));
                    return;
                }
                for (int index = 0; index < notes.size(); ++index) {
                    if (notes.at(index))
                        replaceOriginalPhonemes(notes.at(index), task->result().phonemes.at(index));
                }
                runtime->divider->update();
                synchronizePieces(runtime);
                schedulePieceStage(runtime, piece, SynthesisTaskType::Parameter, options);
            } else if (type == SynthesisTaskType::Parameter) {
                if (m_commitPending) {
                    m_commitPending = false;
                    processCommittedChanges();
                    if (!piece || !runtime->clip || SynthesisPiecePrivate::get(piece)->revision != revision)
                        return;
                }
                ensureParameterNodes(runtime, task->result().parameters.keys());
                writeParameterOrigins(windowHandle()->cast<Core::ProjectWindowInterface>(), runtime->clip, piece, task->result().parameters);
                schedulePieceStage(runtime, piece, SynthesisTaskType::Audio, options);
            } else {
                QString error;
                if (!installAudio(runtime, piece, task->result().audioFilePath, &error)) {
                    notifyFailure(piece, error);
                    return;
                }
                auto d = SynthesisPiecePrivate::get(piece);
                d->state = SynthesisPiece::Ready;
                d->audioFilePath = task->result().audioFilePath;
                d->errorMessage.clear();
                Q_EMIT piece->audioFileChanged();
                Q_EMIT piece->stateChanged();
                Q_EMIT m_context->pieceChanged(piece);
                updateCounts();
            }
        });
        updatePriorities();
    }

    void SynthesisProjectAddOn::bindPieceTask(ClipRuntime *, SynthesisPiece *piece, SynthesisTask *task, quint64 revision, const SynthesisTaskOptions &) {
        auto d = SynthesisPiecePrivate::get(piece);
        d->state = SynthesisPiece::Queued;
        d->currentTaskType = task->type();
        d->errorMessage.clear();
        Q_EMIT piece->stateChanged();
        Q_EMIT m_context->pieceChanged(piece);
        connect(task, &SynthesisTask::stateChanged, this, [this, piece = QPointer<SynthesisPiece>(piece), task, revision] {
            if (!piece || SynthesisPiecePrivate::get(piece)->revision != revision)
                return;
            auto d = SynthesisPiecePrivate::get(piece);
            if (task->state() == SynthesisTask::Running)
                d->state = SynthesisPiece::Synthesizing;
            Q_EMIT piece->stateChanged();
            Q_EMIT m_context->pieceChanged(piece);
            updateCounts();
        });
        updateCounts();
    }

    void SynthesisProjectAddOn::resynthesizeProject(SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        for (auto runtime : m_clips) {
            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
            invalidate(runtime, fromType, piecesForClip(runtime->clip), options);
        }
        updatePriorities();
    }

    void SynthesisProjectAddOn::resynthesizeClip(dspx::SingingClip *clip, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (auto runtime = m_clips.value(clip)) {
            configureDivider(runtime->divider);
            runtime->divider->update();
            synchronizePieces(runtime);
            invalidate(runtime, fromType, piecesForClip(clip), options);
        }
        updatePriorities();
    }

    void SynthesisProjectAddOn::resynthesizePiece(SynthesisPiece *piece, SynthesisTaskType fromType, const SynthesisTaskOptions &options) {
        if (!piece)
            return;
        auto runtime = m_clips.value(piece->singingClip());
        if (!runtime)
            return;
        configureDivider(runtime->divider);
        runtime->divider->update();
        synchronizePieces(runtime);
        if (!runtime->pieces.values().contains(piece))
            return;
        const auto affected = fromType <= SynthesisTaskType::Phoneme ? piecesForClip(runtime->clip)
                                                                     : QList<SynthesisPiece *>{piece};
        invalidate(runtime, fromType, affected, options);
        updatePriorities();
    }

    bool SynthesisProjectAddOn::cancelPiece(SynthesisPiece *piece) {
        if (!piece)
            return false;
        auto task = m_pieceTasks.value(piece);
        if (task && !task->isFinished()) {
            auto d = SynthesisPiecePrivate::get(piece);
            ++d->revision;
            d->state = SynthesisPiece::Stale;
            d->errorMessage.clear();
            Q_EMIT piece->stateChanged();
            Q_EMIT m_context->pieceChanged(piece);
            const bool canceled = m_taskManager->cancel(task);
            updateCounts();
            return canceled;
        }
        auto runtime = m_clips.value(piece->singingClip());
        if (!runtime || !runtime->languageTask || runtime->languageTask->isFinished()) {
            return false;
        }
        ++runtime->revision;
        for (auto related : runtime->pieces) {
            auto d = SynthesisPiecePrivate::get(related);
            ++d->revision;
            d->state = SynthesisPiece::Stale;
            d->errorMessage.clear();
            Q_EMIT related->stateChanged();
            Q_EMIT m_context->pieceChanged(related);
        }
        const bool canceled = m_taskManager->cancel(runtime->languageTask);
        updateCounts();
        return canceled;
    }

    void SynthesisProjectAddOn::cancelAll() {
        for (auto piece : m_pieceTasks.keys())
            cancelPiece(piece);
        for (auto runtime : m_clips) {
            if (runtime->languageTask && !runtime->languageTask->isFinished()) {
                ++runtime->revision;
                for (auto piece : runtime->pieces) {
                    auto d = SynthesisPiecePrivate::get(piece);
                    ++d->revision;
                    d->state = SynthesisPiece::Stale;
                    d->errorMessage.clear();
                    Q_EMIT piece->stateChanged();
                    Q_EMIT m_context->pieceChanged(piece);
                }
                m_taskManager->cancel(runtime->languageTask);
            }
        }
        updateCounts();
    }

    void SynthesisProjectAddOn::updatePriorities() {
        if (!m_taskManager)
            return;
        const int playhead = windowHandle()->cast<Core::ProjectWindowInterface>()->projectTimeline()->position();
        const auto priorityForRange = [playhead](double start, double end) {
            if (playhead >= start && playhead < end)
                return 1000000000;
            if (start >= playhead)
                return 500000000 - static_cast<int>(start - playhead);
            return 100000000 - static_cast<int>(playhead - end);
        };
        for (auto runtime : m_clips) {
            if (!runtime->clip || !runtime->languageTask ||
                runtime->languageTask->state() != SynthesisTask::Queued) {
                continue;
            }
            const double start = runtime->clip->position();
            const double end = start + runtime->clip->clipLength();
            m_taskManager->setPriority(runtime->languageTask, priorityForRange(start, end));
        }
        for (auto it = m_pieceTasks.cbegin(); it != m_pieceTasks.cend(); ++it) {
            auto piece = it.key();
            auto task = it.value();
            if (!piece || !task || task->state() != SynthesisTask::Queued)
                continue;
            const double start = piece->singingClip()->start() + piece->position();
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
        runtime->watcher->takeChanges();
        return true;
    }

    void SynthesisProjectAddOn::removeAudio(SynthesisPiece *piece) {
        auto binding = m_audioBindings.take(piece);
        if (!binding)
            return;
        if (binding->trackContext && binding->series && binding->clip.isValid())
            binding->series->removeClip(binding->clip);
        if (binding->mixer && binding->source)
            binding->mixer->removeSource(binding->source);
        delete binding->mixer;
        delete binding->source;
        delete binding;
        auto d = SynthesisPiecePrivate::get(piece);
        if (!d->audioFilePath.isEmpty()) {
            d->audioFilePath.clear();
            Q_EMIT piece->audioFileChanged();
        }
    }

    bool SynthesisProjectAddOn::installAudio(ClipRuntime *runtime, SynthesisPiece *piece, const QString &filePath, QString *errorMessage) {
        removeAudio(piece);
        auto clipSequence = runtime->clip->clipSequence();
        auto track = clipSequence ? clipSequence->track() : nullptr;
        auto trackContext = track ? Audio::TrackAudioContext::of(track) : nullptr;
        auto series = trackContext ? trackContext->clipSeries() : nullptr;
        auto io = Audio::GlobalAudioContext::formatManager()->getFormatLoad(filePath);
        if (!series || !io) {
            delete io;
            if (errorMessage)
                *errorMessage = tr("The synthesized audio file could not be opened.");
            return false;
        }
        auto source = new talcs::AudioFormatInputSource(io, true);
        source->setStereoize(true);
        auto mixer = new talcs::PositionableMixerAudioSource;
        mixer->addSource(source);
        mixer->setGain(static_cast<float>(runtime->clip->gain()));
        mixer->setPan(static_cast<float>(runtime->clip->pan()));
        mixer->setSilentFlags(runtime->clip->mute() ? -1 : 0);
        connect(runtime->clip, &dspx::Clip::gainChanged, mixer, &talcs::PositionableMixerAudioSource::setGain);
        connect(runtime->clip, &dspx::Clip::panChanged, mixer, &talcs::PositionableMixerAudioSource::setPan);
        connect(runtime->clip, &dspx::Clip::muteChanged, mixer, [mixer](bool mute) {
            mixer->setSilentFlags(mute ? -1 : 0);
        });
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto timeline = window->projectTimeline()->musicTimeline();
        const double pieceStartTick = runtime->clip->start() + piece->position();
        const double pieceEndTick = pieceStartTick + piece->length();
        const double visibleStartTick = runtime->clip->position();
        const double visibleEndTick = visibleStartTick + runtime->clip->clipLength();
        const double startTick = std::max(pieceStartTick, visibleStartTick);
        const double endTick = std::min(pieceEndTick, visibleEndTick);
        if (endTick <= startTick) {
            mixer->removeSource(source);
            delete mixer;
            delete source;
            if (errorMessage)
                *errorMessage = tr("The synthesized piece is outside the visible clip range.");
            return false;
        }
        double sampleRate = trackContext->trackMixer()->sampleRate();
        if (qFuzzyIsNull(sampleRate))
            sampleRate = Audio::GlobalAudioContext::sampleRate();
        const double pieceStartSeconds = tickSeconds(timeline, pieceStartTick);
        const double startSeconds = tickSeconds(timeline, startTick);
        const double endSeconds = tickSeconds(timeline, endTick);
        const qint64 position = static_cast<qint64>(std::llround(startSeconds * sampleRate));
        const qint64 sourceStart = static_cast<qint64>(std::llround((startSeconds - pieceStartSeconds) * sampleRate));
        const qint64 length = std::max<qint64>(1, static_cast<qint64>(std::llround((endSeconds - startSeconds) * sampleRate)));
        const auto clipView = series->insertClip(mixer, position, sourceStart, length);
        if (!clipView.isValid()) {
            mixer->removeSource(source);
            delete mixer;
            delete source;
            if (errorMessage)
                *errorMessage = tr("The synthesized audio could not be inserted into the track mixer.");
            return false;
        }
        m_audioBindings.insert(piece, new AudioBinding{trackContext, series, clipView, mixer, source});
        return true;
    }

    void SynthesisProjectAddOn::notifyFailure(SynthesisPiece *piece, const QString &message) {
        if (!piece)
            return;
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
