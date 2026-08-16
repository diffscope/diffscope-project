// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisParameterEvaluator.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

#include <QList>
#include <QVariant>

#include <opendspx/anchornode.h>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/Parameter.h>
#include <opendspx/interpolator/parameterinterpolator.h>

namespace Synth::Internal {

    namespace {

        struct AnchorCurveSegment {
            int firstTick{};
            int lastTick{};
            opendspx::ParameterInterpolator interpolator;
        };

        struct FreeValueSpan {
            int firstIndex{};
            QList<QVariant> values;
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
        m_data->editedAnchors = buildAnchorCurve(parameter ? parameter->anchorEdited() : nullptr);
        m_data->transformAnchors = buildAnchorCurve(parameter ? parameter->anchorTransform() : nullptr);
        m_data->editedArray = parameter ? parameter->freeEdited() : nullptr;
        m_data->originalArray = parameter ? parameter->original() : nullptr;
        m_data->transformArray = parameter ? parameter->freeTransform() : nullptr;
    }

    SynthesisParameterEvaluator::~SynthesisParameterEvaluator() = default;

    double SynthesisParameterEvaluator::evaluate(double tick, double fallback) const {
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
        return *base * (transform ? *transform / 1000.0 : 1.0);
    }

}
