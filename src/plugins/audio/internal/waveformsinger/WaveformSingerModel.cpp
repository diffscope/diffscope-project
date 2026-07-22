#include "WaveformSingerModel.h"

#include <algorithm>
#include <cmath>

namespace Audio::Internal {

    namespace {

        constexpr double secondsPerMinute = 60.0;

        const WaveformSingerTempoSegment &segmentForTick(const WaveformSingerTempoSnapshot &snapshot, double tick) {
            const auto right = std::upper_bound(snapshot.segments.begin(), snapshot.segments.end(), tick,
                                                [](double value, const WaveformSingerTempoSegment &segment) {
                                                    return value < segment.tick;
                                                });
            return right == snapshot.segments.begin() ? snapshot.segments.front() : *(right - 1);
        }

        const WaveformSingerTempoSegment &segmentForSeconds(const WaveformSingerTempoSnapshot &snapshot, double seconds) {
            const auto right = std::upper_bound(snapshot.segments.begin(), snapshot.segments.end(), seconds,
                                                [](double value, const WaveformSingerTempoSegment &segment) {
                                                    return value < segment.seconds;
                                                });
            return right == snapshot.segments.begin() ? snapshot.segments.front() : *(right - 1);
        }

        double completeWeight(const std::vector<double> &weights, std::size_t index) {
            return index < weights.size() ? weights[index] : 0.0;
        }

    }

    double WaveformSingerTempoSnapshot::tickToSeconds(double tick) const {
        if (segments.empty() || ticksPerQuarter <= 0) {
            return 0.0;
        }
        const auto &segment = segmentForTick(*this, tick);
        const auto secondsPerTick = secondsPerMinute / (segment.beatsPerMinute * ticksPerQuarter);
        return segment.seconds + (tick - segment.tick) * secondsPerTick;
    }

    double WaveformSingerTempoSnapshot::secondsToTick(double seconds) const {
        if (segments.empty() || ticksPerQuarter <= 0) {
            return 0.0;
        }
        const auto &segment = segmentForSeconds(*this, seconds);
        const auto ticksPerSecond = segment.beatsPerMinute * ticksPerQuarter / secondsPerMinute;
        return segment.tick + (seconds - segment.seconds) * ticksPerSecond;
    }

    std::optional<double> WaveformSingerFreeCurve::evaluate(double tick) const {
        if (tick < 0.0 || size <= 0) {
            return std::nullopt;
        }
        const double index = tick / step;
        const auto leftIndex = static_cast<int>(std::floor(index));
        if (leftIndex < 0 || leftIndex >= size) {
            return std::nullopt;
        }

        const auto valueAt = [this](int i) -> std::optional<double> {
            if (i < 0 || i >= size) {
                return std::nullopt;
            }
            const auto blockIndex = static_cast<std::size_t>(i / blockSize);
            const auto itemIndex = i % blockSize;
            const auto &block = *blocks[blockIndex];
            if ((block.valid[static_cast<std::size_t>(itemIndex / 64)] &
                 (std::uint64_t{1} << (itemIndex % 64))) == 0) {
                return std::nullopt;
            }
            return static_cast<double>(block.values[static_cast<std::size_t>(itemIndex)]);
        };

        const auto left = valueAt(leftIndex);
        const double fraction = index - leftIndex;
        if (fraction == 0.0) {
            return left;
        }
        const auto right = valueAt(leftIndex + 1);
        if (!left || !right) {
            return std::nullopt;
        }
        return *left + (*right - *left) * fraction;
    }

    std::optional<double> WaveformSingerAnchorCurve::evaluate(double tick) const {
        const auto right = std::lower_bound(segments.begin(), segments.end(), tick,
                                            [](const Segment &segment, double value) {
                                                return segment.lastTick < value;
                                            });
        if (right == segments.end() || tick < right->firstTick) {
            return std::nullopt;
        }
        return right->interpolator->evaluate(tick);
    }

    std::optional<double> WaveformSingerParameterSnapshot::evaluate(double tick) const {
        auto base = anchorEdited.evaluate(tick);
        if (!base) {
            base = freeEdited.evaluate(tick);
        }
        if (!base) {
            base = original.evaluate(tick);
        }
        if (!base) {
            return std::nullopt;
        }

        auto transform = anchorTransform.evaluate(tick);
        if (!transform) {
            transform = freeTransform.evaluate(tick);
        }
        return *base * (transform ? *transform / 1000.0 : 1.0);
    }

    void WaveformSingerVoiceSnapshot::evaluate(double tick, std::array<double, waveformSingerTypeCount> &result) const {
        result.fill(0.0);
        if (roots.empty()) {
            return;
        }

        const WaveformSingerMixAnchor *left = nullptr;
        const WaveformSingerMixAnchor *right = nullptr;
        if (!anchors.empty()) {
            const auto rightIt = std::lower_bound(anchors.begin(), anchors.end(), tick,
                                                  [](const WaveformSingerMixAnchor &anchor, double value) {
                                                      return anchor.tick < value;
                                                  });
            if (rightIt == anchors.begin()) {
                left = right = &*rightIt;
            } else if (rightIt == anchors.end()) {
                left = right = &anchors.back();
            } else if (rightIt->tick == tick) {
                left = right = &*rightIt;
            } else {
                right = &*rightIt;
                left = &*(rightIt - 1);
            }
        }

        const double factor = left && right && left != right
            ? (tick - left->tick) / static_cast<double>(right->tick - left->tick)
            : 0.0;
        const auto equalWeight = 1.0 / static_cast<double>(roots.size());
        for (std::size_t rootIndex = 0; rootIndex < roots.size(); ++rootIndex) {
            double rootWeight = equalWeight;
            if (left) {
                const auto leftWeight = completeWeight(left->weights, rootIndex);
                const auto rightWeight = completeWeight(right->weights, rootIndex);
                rootWeight = leftWeight + (rightWeight - leftWeight) * factor;
            }
            for (int typeIndex = 0; typeIndex < waveformSingerTypeCount; ++typeIndex) {
                result[static_cast<std::size_t>(typeIndex)] +=
                    rootWeight * roots[rootIndex][static_cast<std::size_t>(typeIndex)];
            }
        }
    }

}
