// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisParameterEvaluator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include <QList>
#include <QVariant>

#include <opendspx/anchornode.h>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/Parameter.h>

#include <coreplugin/ArchitectureInfo.h>

namespace Synth::Internal {

    namespace {

        struct AnchorValue {
            opendspx::AnchorNode::Interpolation interpolation{opendspx::AnchorNode::Interpolation::None};
            int tick{};
            double value{};
        };

        struct AnchorCurveSegment {
            int firstTick{};
            int lastTick{};
            std::vector<AnchorValue> anchors;
        };

        struct FreeValueSpan {
            int firstIndex{};
            std::vector<std::optional<double>> values;
        };

        double anchorSlope(const std::vector<AnchorValue> &anchors, std::size_t index) {
            const auto left = index == 0 ? index : index - 1;
            const auto right = index + 1 < anchors.size() ? index + 1 : index;
            const double tickDistance = anchors[right].tick - anchors[left].tick;
            return tickDistance == 0.0 ? 0.0
                                       : (anchors[right].value - anchors[left].value) / tickDistance;
        }

        std::vector<AnchorCurveSegment> buildAnchorCurve(const dspx::AnchorNodeSequence *sequence,
                                                         int minimumTick, int maximumTick) {
            std::vector<AnchorCurveSegment> result;
            if (!sequence || maximumTick < minimumTick) {
                return result;
            }
            const int slicePosition = std::max(0, minimumTick);
            const qint64 sliceEnd = std::max(static_cast<qint64>(slicePosition) + 1,
                                             static_cast<qint64>(maximumTick) + 1);
            const int sliceLength = static_cast<int>(std::min(
                sliceEnd - slicePosition,
                static_cast<qint64>(std::numeric_limits<int>::max())));
            std::vector<AnchorValue> current;
            const auto appendSegment = [&result, &current] {
                if (current.empty()) {
                    return;
                }
                const int firstTick = current.front().tick;
                const int lastTick = current.back().tick;
                result.push_back({
                    firstTick,
                    lastTick,
                    std::move(current),
                });
                current.clear();
            };
            for (const auto node : sequence->sliceEffective(slicePosition, sliceLength)) {
                current.push_back({
                    static_cast<opendspx::AnchorNode::Interpolation>(node->interpolationMode()),
                    node->x(),
                    Core::ParameterInfo::fromDspxModelValue(node->y()),
                });
                if (node->interpolationMode() == dspx::AnchorNode::None) {
                    appendSegment();
                }
            }
            appendSegment();
            return result;
        }

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
            const auto source = array->slice(firstIndex, static_cast<int>(requestedLength));
            std::vector<std::optional<double>> values;
            values.reserve(static_cast<std::size_t>(source.size()));
            for (const auto &value : source) {
                values.push_back(value.isValid() && !value.isNull()
                                     ? std::optional(Core::ParameterInfo::fromDspxModelValue(value.toInt()))
                                     : std::nullopt);
            }
            return {firstIndex, std::move(values)};
        }

        std::optional<double> freeValue(const FreeValueSpan &span, double tick) {
            if (tick < 0.0) {
                return std::nullopt;
            }
            const double index = tick / dspx::FreeValueDataArray::step();
            const int leftIndex = static_cast<int>(std::floor(index));
            const int offset = leftIndex - span.firstIndex;
            if (offset < 0 || static_cast<std::size_t>(offset) >= span.values.size()) {
                return std::nullopt;
            }
            const auto &leftValue = span.values[static_cast<std::size_t>(offset)];
            if (!leftValue) {
                return std::nullopt;
            }
            const double left = *leftValue;
            const double fraction = index - leftIndex;
            if (qFuzzyIsNull(fraction)) {
                return left;
            }
            if (static_cast<std::size_t>(offset + 1) >= span.values.size()) {
                return std::nullopt;
            }
            const auto &rightValue = span.values[static_cast<std::size_t>(offset + 1)];
            if (!rightValue) {
                return std::nullopt;
            }
            return left + (*rightValue - left) * fraction;
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
            const auto &anchors = segment->anchors;
            const auto upper = std::lower_bound(anchors.begin(), anchors.end(), tick,
                                                [](const AnchorValue &anchor, double value) {
                                                    return anchor.tick < value;
                                                });
            if (upper == anchors.begin())
                return upper->value;
            if (upper == anchors.end())
                return anchors.back().value;
            if (upper->tick == tick)
                return upper->value;

            const auto rightIndex = static_cast<std::size_t>(upper - anchors.begin());
            const auto leftIndex = rightIndex - 1;
            const auto &left = anchors[leftIndex];
            const auto &right = anchors[rightIndex];
            const double duration = right.tick - left.tick;
            if (duration <= 0.0)
                return left.value;
            const double position = (tick - left.tick) / duration;
            if (left.interpolation != opendspx::AnchorNode::Interpolation::Hermite)
                return std::clamp(left.value + (right.value - left.value) * position, 0.0, 1.0);

            const double position2 = position * position;
            const double position3 = position2 * position;
            return std::clamp(
                (2.0 * position3 - 3.0 * position2 + 1.0) * left.value +
                    (position3 - 2.0 * position2 + position) * duration *
                        anchorSlope(anchors, leftIndex) +
                    (-2.0 * position3 + 3.0 * position2) * right.value +
                    (position3 - position2) * duration * anchorSlope(anchors, rightIndex),
                0.0, 1.0);
        }

    }

    struct SynthesisParameterEvaluator::Data {
        const FreeValueSpan &values(const dspx::FreeValueDataArray *array, std::optional<FreeValueSpan> &cached) const {
            if (!cached) {
                cached = captureFreeValues(array, minimumTick, maximumTick);
            }
            return *cached;
        }

        dspx::Parameter *parameter{};
        int minimumTick{};
        int maximumTick{};
        std::vector<AnchorCurveSegment> editedAnchors;
        std::vector<AnchorCurveSegment> transformAnchors;
        const dspx::FreeValueDataArray *editedArray{};
        const dspx::FreeValueDataArray *originalArray{};
        const dspx::FreeValueDataArray *transformArray{};
        mutable std::optional<FreeValueSpan> editedValues;
        mutable std::optional<FreeValueSpan> originalValues;
        mutable std::optional<FreeValueSpan> transformValues;
    };

    SynthesisParameterEvaluator::SynthesisParameterEvaluator(dspx::Parameter *parameter, int minimumTick, int maximumTick)
        : m_data(std::make_unique<Data>()) {
        m_data->parameter = parameter;
        m_data->minimumTick = minimumTick;
        m_data->maximumTick = maximumTick;
        m_data->editedAnchors = buildAnchorCurve(parameter ? parameter->anchorEdited() : nullptr,
                                                 minimumTick, maximumTick);
        m_data->transformAnchors = buildAnchorCurve(parameter ? parameter->anchorTransform() : nullptr,
                                                    minimumTick, maximumTick);
        m_data->editedArray = parameter ? parameter->freeEdited() : nullptr;
        m_data->originalArray = parameter ? parameter->original() : nullptr;
        m_data->transformArray = parameter ? parameter->freeTransform() : nullptr;
    }

    SynthesisParameterEvaluator::~SynthesisParameterEvaluator() = default;

    double SynthesisParameterEvaluator::evaluate(double tick, double fallback) const {
        if (!std::isfinite(fallback))
            fallback = 0.0;
        fallback = std::clamp(fallback, 0.0, 1.0);
        if (!m_data->parameter) {
            return fallback;
        }
        auto base = anchorValue(m_data->editedAnchors, tick);
        if (!base) {
            base = freeValue(m_data->values(m_data->editedArray, m_data->editedValues), tick);
        }
        if (!base) {
            base = freeValue(m_data->values(m_data->originalArray, m_data->originalValues), tick);
        }
        if (!base) {
            base = fallback;
        }
        auto transform = anchorValue(m_data->transformAnchors, tick);
        if (!transform) {
            transform = freeValue(m_data->values(m_data->transformArray, m_data->transformValues), tick);
        }
        return std::clamp(*base * (transform ? *transform * 2.0 : 1.0), 0.0, 1.0);
    }

}
