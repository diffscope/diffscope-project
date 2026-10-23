// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TempoExtractionPlanner.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <set>
#include <tuple>
#include <utility>

#include <QCoreApplication>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

namespace Synth::Internal {

    namespace {

        struct TempoSection {
            int tick;
            double seconds;
            double bpm;
        };

        // Fractional tick conversion precedes integer boundary quantization.
        class TempoMapping {
        public:
            TempoMapping(const QMap<int, double> &tempi, int resolution) : m_resolution(resolution) {
                double seconds = 0;
                for (auto it = tempi.cbegin(); it != tempi.cend(); ++it) {
                    if (!m_sections.isEmpty()) {
                        const auto &previous = m_sections.last();
                        seconds += (it.key() - previous.tick) * 60.0 / (resolution * previous.bpm);
                    }
                    m_sections.append({it.key(), seconds, it.value()});
                }
            }

            double seconds(double tick) const {
                auto it = std::upper_bound(m_sections.cbegin(), m_sections.cend(), tick, [](double value, const TempoSection &section) { return value < section.tick; });
                const auto &section = it == m_sections.cbegin() ? *it : *std::prev(it);
                return section.seconds + (tick - section.tick) * 60.0 / (m_resolution * section.bpm);
            }

            double tick(double seconds) const {
                auto it = std::upper_bound(m_sections.cbegin(), m_sections.cend(), seconds, [](double value, const TempoSection &section) { return value < section.seconds; });
                const auto &section = it == m_sections.cbegin() ? *it : *std::prev(it);
                return section.tick + (seconds - section.seconds) * m_resolution * section.bpm / 60.0;
            }

        private:
            int m_resolution;
            QVector<TempoSection> m_sections;
        };

        double tempoAt(const QMap<int, double> &tempi, int tick) {
            auto it = tempi.upperBound(tick);
            return it == tempi.cbegin() ? it.value() : std::prev(it).value();
        }

        double median(QVector<double> values) {
            if (values.isEmpty())
                return 0;
            const auto middle = values.begin() + values.size() / 2;
            std::nth_element(values.begin(), middle, values.end());
            return values.size() % 2 ? *middle : (*middle + *std::max_element(values.begin(), middle)) / 2;
        }

        struct BeatGrid {
            QVector<double> positions;
            QVector<bool> usable;
            double period{};
            double phase{};
        };

        double huber(double value) {
            const auto magnitude = std::abs(value);
            return magnitude <= 1 ? value * value : 2 * magnitude - 1;
        }

        bool refinePeriod(const TempoExtractionInput &input, BeatGrid &grid, bool redescending, const std::shared_ptr<std::atomic_bool> &cancelled) {
            QVector<double> phases;
            for (qsizetype i = 0; i < input.beats.size(); ++i) {
                if (grid.usable[i])
                    phases.append(input.beats[i].seconds - grid.positions[i] * grid.period);
            }
            if (phases.size() < 2)
                return false;
            grid.phase = median(phases);
            for (int iteration = 0; iteration < 12; ++iteration) {
                if (cancelled->load())
                    return false;
                QVector<double> residuals;
                for (qsizetype i = 0; i < input.beats.size(); ++i) {
                    if (grid.usable[i])
                        residuals.append(std::abs(input.beats[i].seconds - grid.phase - grid.positions[i] * grid.period));
                }
                const auto threshold = std::max(grid.period * 0.02, (redescending ? 4.685 : 1.5) * 1.4826 * median(residuals));
                QVector<double> weights(input.beats.size());
                double total = 0;
                double positionMean = 0;
                double secondsMean = 0;
                for (qsizetype i = 0; i < input.beats.size(); ++i) {
                    if (!grid.usable[i])
                        continue;
                    const auto residual = std::abs(input.beats[i].seconds - grid.phase - grid.positions[i] * grid.period) / threshold;
                    weights[i] = redescending ? std::pow(std::max(0.0, 1 - residual * residual), 2) : (residual <= 1 ? 1 : 1 / residual);
                    total += weights[i];
                    positionMean += weights[i] * grid.positions[i];
                    secondsMean += weights[i] * input.beats[i].seconds;
                }
                if (total <= 0)
                    return false;
                positionMean /= total;
                secondsMean /= total;
                double numerator = 0;
                double denominator = 0;
                for (qsizetype i = 0; i < input.beats.size(); ++i) {
                    const auto position = grid.positions[i] - positionMean;
                    numerator += weights[i] * position * (input.beats[i].seconds - secondsMean);
                    denominator += weights[i] * position * position;
                }
                if (denominator <= 0)
                    return false;
                grid.period = numerator / denominator;
                grid.phase = secondsMean - grid.period * positionMean;
                if (!std::isfinite(grid.period) || grid.period <= 0 || !std::isfinite(grid.phase))
                    return false;
            }
            return true;
        }

        double quartersPerBar(const TempoExtractionInput &input) {
            SVS::MusicTimeline timeline(input.resolution);
            QList<QPair<int, SVS::MusicTimeSignature>> signatures;
            for (auto it = input.timeSignatures.cbegin(); it != input.timeSignatures.cend(); ++it)
                signatures.append({it.key(), it.value()});
            timeline.setTimeSignatures(signatures);
            const auto signature = timeline.timeSignatureAt(timeline.create(0, 0, input.position).measure());
            return signature.ticksPerBar(input.resolution) / static_cast<double>(input.resolution);
        }

        BeatGrid recoverBeatGrid(const TempoExtractionInput &input, const std::shared_ptr<std::atomic_bool> &cancelled) {
            QVector<double> intervals;
            QVector<double> barIntervals;
            QVector<qsizetype> downbeats;
            qsizetype lastDownbeat = -1;
            for (qsizetype i = 0; i < input.beats.size(); ++i) {
                if (input.beats[i].downbeat) {
                    downbeats.append(i);
                    if (lastDownbeat >= 0 && input.beats[i].seconds > input.beats[lastDownbeat].seconds)
                        barIntervals.append(input.beats[i].seconds - input.beats[lastDownbeat].seconds);
                    lastDownbeat = i;
                }
                if (i > 0 && input.beats[i].seconds > input.beats[i - 1].seconds)
                    intervals.append(input.beats[i].seconds - input.beats[i - 1].seconds);
            }
            const auto barLength = quartersPerBar(input);
            const auto barSlots = qRound(barLength * 8);
            auto period = median(intervals);
            if (barIntervals.size() >= 2 && barLength > 0)
                period = median(barIntervals) / barLength;
            if (period <= 0 || !std::isfinite(period) || barSlots <= 0 || barSlots > 256)
                return {};

            struct State {
                double cost;
                double period;
                qint64 position;
                qsizetype lastBeat;
                int parent;
                bool accepted;
                qint64 stride;
            };
            struct Trace {
                qint64 position;
                int parent;
                bool accepted;
            };
            constexpr qsizetype beamWidth = 64;
            QVector<State> states;
            for (int phase = 0; phase < barSlots; ++phase)
                states.append({input.beats.first().downbeat && phase != 0 ? 2.0 : 0.0, period, phase, 0, -1, true, 8});
            QVector<QVector<Trace>> history;
            history.reserve(input.beats.size());
            QVector<Trace> initial;
            for (const auto &state : states)
                initial.append({state.position, -1, true});
            history.append(initial);
            // Segmentation smoothing does not change observation acceptance or musical beat numbering.
            const auto tempoScale = std::log2(1.095);
            const auto octaveEquivalent = [tempoScale](double ratio) {
                const auto distance = std::log2(ratio);
                const auto octave = std::round(distance);
                return octave != 0 && std::abs(distance - octave) <= 2 * tempoScale;
            };
            // A finite bar-phase penalty permits incorrect downbeat labels without cumulative beat-count drift.
            for (qsizetype i = 1; i < input.beats.size(); ++i) {
                if (cancelled->load())
                    return {};
                const auto firstDownbeat = std::lower_bound(downbeats.cbegin(), downbeats.cend(), i) - downbeats.cbegin();
                QVector<double> followingBars;
                for (auto j = firstDownbeat; j + 1 < downbeats.size() && j < firstDownbeat + 5; ++j)
                    followingBars.append(input.beats[downbeats[j + 1]].seconds - input.beats[downbeats[j]].seconds);
                const auto followingPeriod = followingBars.size() >= 3 ? median(followingBars) / barLength : 0;
                QVector<State> pending;
                pending.reserve(states.size() * 18);
                for (qsizetype parent = 0; parent < states.size(); ++parent) {
                    const auto &old = states[parent];
                    pending.append({old.cost + (input.beats[i].downbeat ? 6 : 4), old.period, old.position, old.lastBeat, static_cast<int>(parent), false, old.stride});
                    const auto duration = input.beats[i].seconds - input.beats[old.lastBeat].seconds;
                    QVector<qint64> strides{1, 2, 4, 8, 16, 32, 64, 128};
                    if (duration / old.period > 1.5) {
                        const auto stride = qRound64(duration / old.period) * 8;
                        if (!strides.contains(stride))
                            strides.append(stride);
                    }
                    for (const auto stride : strides) {
                        const auto nextPeriod = duration * 8 / stride;
                        if (nextPeriod < 0.015 || nextPeriod > 24 || !std::isfinite(nextPeriod))
                            continue;
                        const auto position = old.position + stride;
                        const auto phaseCost = input.beats[i].downbeat && position % barSlots != 0 ? 2.0 : 0.0;
                        const auto cost = old.cost + phaseCost + huber(std::log2(nextPeriod / old.period) / tempoScale) + 0.15 * std::abs(std::log2(static_cast<double>(stride) / old.stride));
                        const auto weight = 1 - std::exp(-stride / 64.0);
                        const auto latentPeriod = old.period * std::exp(weight * std::log(nextPeriod / old.period));
                        pending.append({cost, latentPeriod, position, i, static_cast<int>(parent), true, stride});
                        // Multiple following bars support abrupt changes. Octave-equivalent intervals use stride changes.
                        if (followingPeriod > 0 && std::abs(std::log2(followingPeriod / old.period)) > tempoScale
                            && !octaveEquivalent(followingPeriod / old.period) && !octaveEquivalent(followingPeriod / period)) {
                            const auto changeCost = old.cost + phaseCost + 6 + huber(std::log2(nextPeriod / followingPeriod) / tempoScale)
                                + 0.15 * std::abs(std::log2(static_cast<double>(stride) / old.stride));
                            pending.append({changeCost, followingPeriod, position, i, static_cast<int>(parent), true, stride});
                        }
                    }
                }
                std::stable_sort(pending.begin(), pending.end(), [](const State &a, const State &b) { return a.cost < b.cost; });
                states.clear();
                std::set<std::tuple<qint64, int, qsizetype, qint64>> seen;
                QVector<Trace> trace;
                for (const auto &candidate : pending) {
                    const auto key = std::make_tuple(candidate.position % barSlots, qRound(std::log2(candidate.period) * 100), candidate.lastBeat, candidate.stride);
                    if (!seen.insert(key).second)
                        continue;
                    states.append(candidate);
                    trace.append({candidate.position, candidate.parent, candidate.accepted});
                    if (states.size() == beamWidth)
                        break;
                }
                history.append(trace);
            }
            BeatGrid grid;
            grid.period = period;
            grid.positions.resize(input.beats.size());
            grid.usable.resize(input.beats.size());
            int path = 0;
            for (qsizetype i = input.beats.size(); i-- > 0;) {
                const auto &trace = history[i][path];
                grid.positions[i] = trace.position / 8.0;
                grid.usable[i] = trace.accepted;
                path = trace.parent;
            }
            if (!refinePeriod(input, grid, false, cancelled))
                return {};
            return grid;
        }

        BeatGrid dominantFixedGrid(const TempoExtractionInput &input, const BeatGrid &tracked, const std::shared_ptr<std::atomic_bool> &cancelled) {
            struct Window {
                qsizetype first;
                qsizetype last;
                double period;
                double duration;
                double error;
            };
            QVector<qsizetype> usable;
            for (qsizetype i = 0; i < input.beats.size(); ++i) {
                if (tracked.usable[i])
                    usable.append(i);
            }
            if (usable.size() < 2)
                return {};
            const auto windowLength = std::max(16.0, (tracked.positions[usable.last()] - tracked.positions[usable.first()]) / 512);
            QVector<Window> windows;
            for (qsizetype first = 0; first + 1 < usable.size();) {
                if (cancelled->load())
                    return {};
                auto last = first + 1;
                while (last + 1 < usable.size() && tracked.positions[usable[last]] - tracked.positions[usable[first]] < windowLength)
                    ++last;
                double meanPosition = 0;
                double meanSeconds = 0;
                for (auto j = first; j <= last; ++j) {
                    meanPosition += tracked.positions[usable[j]];
                    meanSeconds += input.beats[usable[j]].seconds;
                }
                const auto count = last - first + 1;
                meanPosition /= count;
                meanSeconds /= count;
                double numerator = 0;
                double denominator = 0;
                for (auto j = first; j <= last; ++j) {
                    const auto position = tracked.positions[usable[j]] - meanPosition;
                    numerator += position * (input.beats[usable[j]].seconds - meanSeconds);
                    denominator += position * position;
                }
                if (denominator > 0) {
                    const auto period = numerator / denominator;
                    if (period > 0) {
                        const auto phase = meanSeconds - period * meanPosition;
                        double error = 0;
                        for (auto j = first; j <= last; ++j) {
                            const auto residual = (input.beats[usable[j]].seconds - phase - tracked.positions[usable[j]] * period) / period;
                            error += residual * residual;
                        }
                        windows.append({usable[first], usable[last], period, input.beats[usable[last]].seconds - input.beats[usable[first]].seconds, std::sqrt(error / count)});
                    }
                }
                first = last;
            }
            if (windows.isEmpty())
                return {};
            const auto stable = std::ranges::any_of(windows, [](const Window &window) { return window.error < 0.1; });
            double bestSupport = -1;
            double mainPeriod = tracked.period;
            // Duration-weighted modal consensus excludes minority tempo changes before period refinement.
            for (const auto &candidate : windows) {
                if (stable && candidate.error >= 0.1)
                    continue;
                double support = 0;
                for (const auto &window : windows) {
                    if (stable && window.error >= 0.1)
                        continue;
                    const auto distance = std::log(window.period / candidate.period) / 0.005;
                    support += window.duration * std::pow(std::max(0.0, 1 - distance * distance), 2);
                }
                if (support > bestSupport) {
                    bestSupport = support;
                    mainPeriod = candidate.period;
                }
            }
            auto supportMask = tracked.usable;
            supportMask.fill(false);
            for (const auto &window : windows) {
                if ((stable && window.error >= 0.1) || std::abs(std::log(window.period / mainPeriod)) >= 0.005)
                    continue;
                for (auto i = window.first; i <= window.last; ++i)
                    supportMask[i] = tracked.usable[i];
            }
            const auto barLength = quartersPerBar(input);
            BeatGrid best;
            double bestCost = std::numeric_limits<double>::infinity();
            for (const auto factor : {0.5, 1.0, 2.0}) {
                BeatGrid candidate = tracked;
                candidate.period = mainPeriod * factor;
                candidate.usable = supportMask;
                for (auto &position : candidate.positions)
                    position = std::round(position / factor);
                if (!refinePeriod(input, candidate, true, cancelled))
                    continue;
                // Reassign observations to the nearest lattice instead of retaining extraction indices.
                for (int iteration = 0; iteration < 4; ++iteration) {
                    for (qsizetype i = 0; i < input.beats.size(); ++i)
                        candidate.positions[i] = std::round((input.beats[i].seconds - candidate.phase) / candidate.period);
                    if (!refinePeriod(input, candidate, true, cancelled))
                        break;
                }
                double bestBarCost = std::numeric_limits<double>::infinity();
                double barPhase = 0;
                for (int phase = 0; phase < qRound(barLength * 8); ++phase) {
                    double strongCost = 0;
                    int strongCount = 0;
                    double lastStrong = -1;
                    for (qsizetype i = 0; i < input.beats.size(); ++i) {
                        if (!supportMask[i] || !input.beats[i].downbeat)
                            continue;
                        ++strongCount;
                        const auto bar = (candidate.positions[i] + phase / 8.0) / barLength;
                        if (std::abs(bar - std::round(bar)) > 1e-6) {
                            strongCost += 4;
                        } else {
                            if (lastStrong >= 0)
                                strongCost += std::min(4.0, 2 * std::abs(std::log2(std::max(0.125, bar - lastStrong))));
                            lastStrong = bar;
                        }
                    }
                    const auto cost = strongCount > 0 ? strongCost / strongCount : 0;
                    if (cost < bestBarCost) {
                        bestBarCost = cost;
                        barPhase = phase / 8.0;
                    }
                }
                candidate.phase -= barPhase * candidate.period;
                for (auto &position : candidate.positions)
                    position += barPhase;
                double cost = 0;
                double duration = 0;
                for (qsizetype i = 0; i < input.beats.size(); ++i) {
                    if (!supportMask[i])
                        continue;
                    const auto weight = ((i > 0 ? input.beats[i].seconds - input.beats[i - 1].seconds : 0)
                                         + (i + 1 < input.beats.size() ? input.beats[i + 1].seconds - input.beats[i].seconds : 0)) / 2;
                    const auto residual = std::abs(input.beats[i].seconds - candidate.phase - candidate.positions[i] * candidate.period) / (candidate.period * 0.08);
                    cost += weight * std::min(1.0, residual * residual);
                    duration += weight;
                }
                cost = (duration > 0 ? cost / duration : 1) + bestBarCost;
                if (cost < bestCost) {
                    bestCost = cost;
                    best = candidate;
                }
            }
            return best;
        }

        BeatGrid normalizeVariableGrid(const TempoExtractionInput &input, const BeatGrid &tracked, const std::shared_ptr<std::atomic_bool> &cancelled) {
            const auto barLength = quartersPerBar(input);
            BeatGrid best = tracked;
            double bestCost = std::numeric_limits<double>::infinity();
            // Variable-tempo phase uses recovered musical positions across the entire region.
            for (const auto factor : {0.5, 1.0, 2.0}) {
                for (int phase = 0; phase < qRound(barLength * 8); ++phase) {
                    if (cancelled->load())
                        return {};
                    const auto shift = phase / 8.0;
                    double cost = 0;
                    qsizetype count = 0;
                    double lastStrong = -1;
                    for (qsizetype i = 0; i < input.beats.size(); ++i) {
                        if (!tracked.usable[i] || !input.beats[i].downbeat)
                            continue;
                        ++count;
                        const auto bar = (tracked.positions[i] / factor + shift) / barLength;
                        if (std::abs(bar - std::round(bar)) > 1e-6) {
                            cost += 4;
                        } else {
                            if (lastStrong >= 0)
                                cost += std::min(4.0, 2 * std::abs(std::log2(std::max(0.125, bar - lastStrong))));
                            lastStrong = bar;
                        }
                    }
                    cost = count > 0 ? cost / count : std::abs(std::log2(factor));
                    if (cost >= bestCost)
                        continue;
                    bestCost = cost;
                    best = tracked;
                    best.period *= factor;
                    best.phase -= shift * best.period;
                    for (auto &position : best.positions)
                        position = position / factor + shift;
                }
            }
            return best;
        }

        QVector<double> segmentBoundaries(const QVector<QPointF> &points, int resolution, const TempoExtractionOptions &options, const std::shared_ptr<std::atomic_bool> &cancelled) {
            if (points.size() < 2)
                return {};
            struct Moments {
                long double weight{};
                long double x{};
                long double y{};
                long double xx{};
                long double xy{};
                long double yy{};
            };
            QVector<Moments> prefix(points.size() + 1);
            const auto origin = points.first();
            const auto quarterTicks = static_cast<double>(resolution);
            for (qsizetype i = 0; i < points.size(); ++i) {
                const auto x = (points[i].x() - origin.x()) / quarterTicks;
                const auto y = points[i].y() - origin.y();
                const auto weight = ((i > 0 ? points[i].x() - points[i - 1].x() : 0)
                                     + (i + 1 < points.size() ? points[i + 1].x() - points[i].x() : 0)) / (2 * quarterTicks);
                auto moments = prefix[i];
                moments.weight += weight;
                moments.x += weight * x;
                moments.y += weight * y;
                moments.xx += weight * x * x;
                moments.xy += weight * x * y;
                moments.yy += weight * y * y;
                prefix[i + 1] = moments;
            }
            QVector<qsizetype> candidates;
            const auto stride = std::max<qsizetype>(1, (points.size() - 1 + 2046) / 2047);
            for (qsizetype i = 0; i < points.size() - 1; i += stride)
                candidates.append(i);
            candidates.append(points.size() - 1);
            QVector<double> costs(candidates.size(), std::numeric_limits<double>::infinity());
            QVector<qsizetype> parents(candidates.size(), -1);
            costs[0] = 0;
            const auto markerPenalty = 0.1 + 1.9 * options.smoothing / 100;
            // Segment costs use musical phase and duration. Observation density does not set the weighting.
            for (qsizetype b = 1; b < candidates.size(); ++b) {
                if (cancelled->load())
                    return {};
                const auto last = candidates[b];
                for (qsizetype a = 0; a < b; ++a) {
                    const auto first = candidates[a];
                    const auto span = (points[last].x() - points[first].x()) / quarterTicks;
                    if (span < options.minimumSegmentBeats && !(a == 0 && b + 1 == candidates.size()))
                        continue;
                    const auto period = (points[last].y() - points[first].y()) / span;
                    if (!std::isfinite(period) || period < 0.06 || period > 6 || !std::isfinite(costs[a]))
                        continue;
                    const auto intercept = points[first].y() - origin.y() - period * (points[first].x() - origin.x()) / quarterTicks;
                    const auto &end = prefix[last + 1];
                    const auto &start = prefix[first];
                    const auto squaredError = (end.yy - start.yy) + period * period * (end.xx - start.xx)
                        + intercept * intercept * (end.weight - start.weight) - 2 * period * (end.xy - start.xy)
                        - 2 * intercept * (end.y - start.y) + 2 * period * intercept * (end.x - start.x);
                    const auto cost = costs[a] + static_cast<double>(std::max(0.0L, squaredError)) / (period * period) + markerPenalty;
                    if (cost < costs[b]) {
                        costs[b] = cost;
                        parents[b] = a;
                    }
                }
            }
            if (parents.last() < 0)
                return {};
            QVector<double> boundaries{(points.last().x() - origin.x()) / quarterTicks};
            for (auto b = candidates.size() - 1; b > 0;) {
                const auto a = parents[b];
                const auto &first = points[candidates[a]];
                boundaries.append((first.x() - origin.x()) / quarterTicks);
                b = a;
            }
            std::ranges::reverse(boundaries);
            return boundaries;
        }

        struct TempoCurve {
            QVector<double> boundaries;
            QVector<double> seconds;
            double cost{std::numeric_limits<double>::infinity()};
        };

        struct TempoObservation {
            double position;
            double seconds;
            double weight;
        };

        TempoCurve fitTempoCurve(const QVector<TempoObservation> &observations, const QVector<double> &boundaries, const std::shared_ptr<std::atomic_bool> &cancelled) {
            TempoCurve curve;
            curve.boundaries = boundaries;
            const auto size = boundaries.size();
            QVector<qsizetype> sections;
            QVector<double> fractions;
            QVector<double> weights;
            sections.reserve(observations.size());
            fractions.reserve(observations.size());
            weights.reserve(observations.size());
            qsizetype section = 0;
            for (const auto &observation : observations) {
                while (section + 2 < size && observation.position >= boundaries[section + 1])
                    ++section;
                sections.append(section);
                fractions.append((observation.position - boundaries[section]) / (boundaries[section + 1] - boundaries[section]));
                weights.append(observation.weight);
            }
            // Linear interpolation gives a tridiagonal normal matrix and continuous segment boundaries.
            for (int iteration = 0; iteration < 8; ++iteration) {
                if (cancelled->load())
                    return {};
                QVector<double> diagonal(size);
                QVector<double> upper(size - 1);
                QVector<double> rhs(size);
                for (qsizetype i = 0; i < observations.size(); ++i) {
                    const auto j = sections[i];
                    const auto right = fractions[i];
                    const auto left = 1 - right;
                    const auto weight = weights[i];
                    diagonal[j] += weight * left * left;
                    diagonal[j + 1] += weight * right * right;
                    upper[j] += weight * left * right;
                    rhs[j] += weight * left * observations[i].seconds;
                    rhs[j + 1] += weight * right * observations[i].seconds;
                }
                for (qsizetype j = 1; j < size; ++j) {
                    if (diagonal[j - 1] <= 1e-12)
                        return {};
                    const auto factor = upper[j - 1] / diagonal[j - 1];
                    diagonal[j] -= factor * upper[j - 1];
                    rhs[j] -= factor * rhs[j - 1];
                }
                curve.seconds.resize(size);
                for (qsizetype j = size; j-- > 0;) {
                    if (diagonal[j] <= 1e-12)
                        return {};
                    curve.seconds[j] = (rhs[j] - (j + 1 < size ? upper[j] * curve.seconds[j + 1] : 0)) / diagonal[j];
                }
                QVector<double> periods;
                for (qsizetype j = 0; j + 1 < size; ++j) {
                    const auto period = (curve.seconds[j + 1] - curve.seconds[j]) / (boundaries[j + 1] - boundaries[j]);
                    if (!std::isfinite(period) || period < 0.06 || period > 6)
                        return {};
                    periods.append(period);
                }
                QVector<double> residuals;
                residuals.reserve(observations.size());
                for (qsizetype i = 0; i < observations.size(); ++i) {
                    const auto j = sections[i];
                    const auto predicted = curve.seconds[j] * (1 - fractions[i]) + curve.seconds[j + 1] * fractions[i];
                    residuals.append((observations[i].seconds - predicted) / periods[j]);
                }
                const auto center = median(residuals);
                QVector<double> deviations;
                deviations.reserve(residuals.size());
                for (const auto residual : residuals)
                    deviations.append(std::abs(residual - center));
                const auto scale = std::max(0.02, 1.4826 * median(deviations));
                curve.cost = 0;
                for (qsizetype i = 0; i < observations.size(); ++i) {
                    const auto residual = std::abs(residuals[i]);
                    weights[i] = observations[i].weight * (residual <= 1.5 * scale ? 1 : 1.5 * scale / residual);
                    curve.cost += observations[i].weight * 0.0025 * huber(residual / 0.05);
                }
            }
            return curve;
        }

        TempoCurve optimizeTempoCurve(const QVector<TempoObservation> &observations, TempoCurve curve, int resolution, int minimumSegmentBeats, const std::shared_ptr<std::atomic_bool> &cancelled) {
            for (const auto step : {1.0, 0.25, 0.125, 1.0 / resolution}) {
                for (int pass = 0; pass < 2; ++pass) {
                    bool changed = false;
                    for (qsizetype j = 1; j + 1 < curve.boundaries.size(); ++j) {
                        if (cancelled->load())
                            return {};
                        auto best = curve;
                        for (const auto offset : {-step, step}) {
                            auto boundaries = curve.boundaries;
                            boundaries[j] += offset;
                            if (boundaries[j] - boundaries[j - 1] < minimumSegmentBeats || boundaries[j + 1] - boundaries[j] < minimumSegmentBeats)
                                continue;
                            auto candidate = fitTempoCurve(observations, boundaries, cancelled);
                            if (candidate.cost < best.cost - 1e-10)
                                best = std::move(candidate);
                        }
                        if (best.cost < curve.cost - 1e-10) {
                            changed = true;
                            curve = std::move(best);
                        }
                    }
                    if (!changed)
                        break;
                }
            }
            return curve;
        }

        struct TempoFit {
            QMap<int, double> tempi;
            double firstSeconds{};
        };

        TempoFit segmentTempo(const QVector<QPointF> &points, int resolution, const TempoExtractionOptions &options, const std::shared_ptr<std::atomic_bool> &cancelled) {
            const auto boundaries = segmentBoundaries(points, resolution, options, cancelled);
            if (boundaries.size() < 2 || cancelled->load())
                return {};
            const auto origin = points.first();
            QVector<TempoObservation> observations;
            observations.reserve(points.size());
            for (qsizetype i = 0; i < points.size(); ++i) {
                const auto weight = ((i > 0 ? points[i].x() - points[i - 1].x() : 0)
                                     + (i + 1 < points.size() ? points[i + 1].x() - points[i].x() : 0)) / (2.0 * resolution);
                observations.append({(points[i].x() - origin.x()) / resolution, points[i].y() - origin.y(), std::min(4.0, weight)});
            }
            auto curve = fitTempoCurve(observations, boundaries, cancelled);
            if (!std::isfinite(curve.cost))
                return {};
            curve = optimizeTempoCurve(observations, std::move(curve), resolution, options.minimumSegmentBeats, cancelled);
            const auto markerPenalty = 0.02 + 0.18 * options.smoothing / 100;
            // Robust joint fitting removes markers supported only by noisy endpoint observations.
            while (curve.boundaries.size() > 2 && !cancelled->load()) {
                TempoCurve best;
                auto bestCost = curve.cost;
                for (qsizetype j = 1; j + 1 < curve.boundaries.size(); ++j) {
                    auto merged = curve.boundaries;
                    merged.removeAt(j);
                    auto candidate = fitTempoCurve(observations, merged, cancelled);
                    if (candidate.cost - markerPenalty < bestCost) {
                        bestCost = candidate.cost - markerPenalty;
                        best = std::move(candidate);
                    }
                }
                if (!std::isfinite(best.cost))
                    break;
                curve = optimizeTempoCurve(observations, std::move(best), resolution, options.minimumSegmentBeats, cancelled);
            }
            if (cancelled->load() || curve.seconds.isEmpty())
                return {};
            // Whole-beat boundaries have lower model complexity if phase evidence does not require sub-beat precision.
            const auto originPosition = origin.x() / resolution;
            for (qsizetype j = 1; j + 1 < curve.boundaries.size(); ++j) {
                auto snapped = curve.boundaries;
                snapped[j] = std::round(originPosition + snapped[j]) - originPosition;
                if (snapped[j] - snapped[j - 1] < options.minimumSegmentBeats || snapped[j + 1] - snapped[j] < options.minimumSegmentBeats)
                    continue;
                auto candidate = fitTempoCurve(observations, snapped, cancelled);
                if (candidate.cost <= curve.cost + markerPenalty / 10)
                    curve = std::move(candidate);
            }
            if (cancelled->load())
                return {};
            TempoFit fit;
            fit.firstSeconds = origin.y() + curve.seconds.first();
            for (qsizetype j = 0; j + 1 < curve.boundaries.size(); ++j) {
                const auto tick = qRound(origin.x() + curve.boundaries[j] * resolution);
                const auto endTick = qRound(origin.x() + curve.boundaries[j + 1] * resolution);
                if (endTick <= tick)
                    return {};
                // The final fit uses the same integer boundary positions as the document.
                const auto bpm = 60.0 * (endTick - tick) / (resolution * (curve.seconds[j + 1] - curve.seconds[j]));
                if (!std::isfinite(bpm) || bpm < 10 || bpm > 1000)
                    return {};
                fit.tempi.insert(tick, bpm);
            }
            return fit;
        }

        QVector<QPointF> beatLines(const TempoExtractionInput &input, const QMap<int, double> &tempi, const std::shared_ptr<std::atomic_bool> &cancelled) {
            SVS::MusicTimeline timeline(input.resolution);
            QList<QPair<int, double>> tempoList;
            tempoList.reserve(tempi.size());
            for (auto it = tempi.cbegin(); it != tempi.cend(); ++it)
                tempoList.append({it.key(), it.value()});
            timeline.setTempi(tempoList);
            QList<QPair<int, SVS::MusicTimeSignature>> signatureList;
            for (auto it = input.timeSignatures.cbegin(); it != input.timeSignatures.cend(); ++it)
                signatureList.append({it.key(), it.value()});
            timeline.setTimeSignatures(signatureList);
            QVector<QPointF> result;
            const auto endMsec = (input.startSeconds + input.durationSeconds) * 1000;
            auto beat = timeline.create(input.startSeconds * 1000).ceilBeat();
            while (beat.isValid() && beat.millisecond() < endMsec && !cancelled->load()) {
                if (beat.millisecond() >= input.startSeconds * 1000)
                    result.append({beat.millisecond() / 1000, beat.beat() == 0 ? 1.0 : 0.0});
                const auto next = beat.nextBeat();
                if (!next.isValid() || next.totalTick() <= beat.totalTick())
                    break;
                beat = next;
            }
            return result;
        }

    }

    TempoExtractionPlan planExtractedTempo(const TempoExtractionInput &input, const TempoExtractionOptions &options, const std::shared_ptr<std::atomic_bool> &cancelled) {
        TempoExtractionPlan plan;
        const auto fail = [&](const char *message) {
            plan.error = QCoreApplication::translate("Synth::Internal::TempoExtractionPreview", message);
            return plan;
        };
        if (input.beats.size() < 2)
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "At least two detected beats are required."));
        if (input.resolution <= 0 || input.position < 0 || input.tempi.isEmpty() || input.tempi.firstKey() != 0
            || !std::isfinite(input.startSeconds) || input.startSeconds < 0 || !std::isfinite(input.durationSeconds) || input.durationSeconds <= 0)
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The audio region is invalid."));
        if (options.referenceBeat < 0 || options.referenceBeat >= input.beats.size() || !std::isfinite(options.smoothing) || options.smoothing < 0 || options.smoothing > 100
            || options.minimumSegmentBeats < 1 || options.minimumSegmentBeats > 64)
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The beat assignment is invalid."));
        double previous = -1;
        for (const auto &beat : input.beats) {
            if (!std::isfinite(beat.seconds) || beat.seconds < previous || beat.seconds < 0 || beat.seconds > input.durationSeconds)
                return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The extractor returned invalid beat data."));
            previous = beat.seconds;
        }
        auto grid = recoverBeatGrid(input, cancelled);
        if (grid.period > 0) {
            grid = options.variable ? normalizeVariableGrid(input, grid, cancelled) : dominantFixedGrid(input, grid, cancelled);
        }
        if (cancelled->load())
            return {};
        if (grid.period <= 0)
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "At least two reliable beats are required."));
        const auto quarterTicks = static_cast<qint64>(input.resolution);
        plan.bpm = options.variable || options.automatic ? 60 / grid.period : options.bpm;
        if (!std::isfinite(plan.bpm) || plan.bpm < 10 || plan.bpm > 1000)
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The tempo must be between 10 and 1000 BPM."));
        const auto firstPosition = grid.positions.first();
        const auto referencePosition = grid.positions[options.referenceBeat];
        auto fixedPeriod = 60 / plan.bpm;
        const auto fittedFirstSeconds = std::max(0.0, grid.phase + firstPosition * grid.period);
        // The fitted reference time is independent of the tempo source.
        auto firstSeconds = options.variable ? input.beats.first().seconds
            : fittedFirstSeconds + (referencePosition - firstPosition) * (grid.period - fixedPeriod);
        if (std::abs(firstSeconds) < 1e-9)
            firstSeconds = 0;
        TempoFit variableFit;
        if (options.variable) {
            QVector<QPointF> points;
            for (qsizetype i = 0; i < input.beats.size(); ++i) {
                const auto tick = grid.positions[i] * quarterTicks;
                if (!std::isfinite(tick) || tick < 0 || tick >= std::numeric_limits<int>::max())
                    return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The extracted tempo exceeds the project time range."));
                if (grid.usable[i] && (points.isEmpty() || tick > points.last().x()))
                    points.append({tick, input.beats[i].seconds});
            }
            variableFit = segmentTempo(points, input.resolution, options, cancelled);
            if (cancelled->load())
                return {};
            if (variableFit.tempi.isEmpty())
                return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "No supported tempo map satisfies the minimum segment length."));
            firstSeconds = std::max(0.0, variableFit.firstSeconds);
            plan.bpm = variableFit.tempi.first();
            fixedPeriod = 60 / plan.bpm;
        }
        if (!std::isfinite(firstSeconds) || firstSeconds < 0)
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The selected tempo cannot align this beat. Choose another reference beat."));
        const auto alignmentReference = [&](bool moveAudio) {
            auto reference = static_cast<double>(options.referenceTick);
            if (options.automaticReference) {
                const auto barTicks = quartersPerBar(input) * quarterTicks;
                const auto estimatedOrigin = input.position + (firstSeconds / fixedPeriod - firstPosition) * quarterTicks;
                auto origin = std::round(estimatedOrigin / barTicks) * barTicks;
                const auto firstTick = origin + firstPosition * quarterTicks;
                if ((moveAudio && firstTick - firstSeconds / fixedPeriod * quarterTicks < 0)
                    || (!moveAudio && firstSeconds > 1e-8 && firstTick <= input.position))
                    origin += std::ceil((estimatedOrigin - origin) / barTicks) * barTicks;
                reference = origin + referencePosition * quarterTicks;
            }
            return reference;
        };
        const auto keepReference = alignmentReference(false);
        if (!std::isfinite(keepReference) || keepReference < 0 || keepReference >= std::numeric_limits<int>::max()) {
            plan.keepAudioPositionError = QCoreApplication::translate("Synth::Internal::TempoExtractionPreview", "The assigned beat cannot be aligned without moving the audio.");
        } else {
            const auto keepOrigin = static_cast<qint64>(qRound(keepReference)) - qRound64(referencePosition * quarterTicks);
            const auto leadTicks = static_cast<double>(keepOrigin + qRound64(firstPosition * quarterTicks)) - input.position;
            if (leadTicks < 0 || (firstSeconds > 1e-8 && leadTicks <= 0) || (firstSeconds <= 1e-8 && leadTicks > 0)) {
                plan.keepAudioPositionError = QCoreApplication::translate("Synth::Internal::TempoExtractionPreview", "The assigned beat cannot be aligned without moving the audio.");
            } else if (leadTicks > 0) {
                const auto leadTempo = 60 * leadTicks / (input.resolution * firstSeconds);
                if (!std::isfinite(leadTempo) || leadTempo < 10 || leadTempo > 1000)
                    plan.keepAudioPositionError = QCoreApplication::translate("Synth::Internal::TempoExtractionPreview", "Keeping the audio position would require an unsupported lead-in tempo.");
            }
        }
        plan.moveAudio = options.moveAudio || !plan.keepAudioPositionError.isEmpty();
        const auto reference = alignmentReference(plan.moveAudio);
        if (!std::isfinite(reference) || reference < 0 || reference >= std::numeric_limits<int>::max())
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "Choose a later reference position."));
        plan.referenceTick = qRound(reference);
        const auto originTick = static_cast<qint64>(plan.referenceTick) - qRound64(referencePosition * quarterTicks);
        for (qsizetype i = 0; i < input.beats.size(); ++i) {
            const auto tick = originTick + qRound64(grid.positions[i] * quarterTicks);
            if (tick < 0 || tick >= std::numeric_limits<int>::max())
                return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "Choose a later reference position."));
            plan.beatTicks.append(static_cast<int>(tick));
        }
        const auto firstTick = plan.beatTicks.first();
        QMap<int, double> fitted;
        if (options.variable) {
            for (auto it = variableFit.tempi.cbegin(); it != variableFit.tempi.cend(); ++it) {
                const auto tick = originTick + it.key();
                if (tick < 0 || tick >= std::numeric_limits<int>::max())
                    return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The extracted tempo exceeds the project time range."));
                fitted.insert(static_cast<int>(tick), it.value());
            }
        } else {
            fitted.insert(firstTick, plan.bpm);
        }
        const auto firstTempo = fitted.first();
        const TempoMapping oldMapping(input.tempi, input.resolution);
        plan.begin = input.position;
        if (plan.moveAudio) {
            const auto estimatedStart = firstTick - firstSeconds * input.resolution * firstTempo / 60;
            if (!std::isfinite(estimatedStart) || estimatedStart < -1e-6)
                return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "Choose a later reference position."));
            plan.begin = std::min(input.position, static_cast<int>(std::floor(std::max(0.0, estimatedStart))));
            fitted.remove(firstTick);
            fitted.insert(plan.begin, firstTempo);
        } else {
            const auto leadTicks = static_cast<double>(firstTick) - input.position;
            if (leadTicks > 0) {
                const auto leadTempo = 60 * leadTicks / (input.resolution * firstSeconds);
                if (std::abs(leadTempo - firstTempo) > 1e-8) {
                    fitted.insert(plan.begin, leadTempo);
                } else {
                    fitted.remove(firstTick);
                    fitted.insert(plan.begin, firstTempo);
                }
            }
        }
        plan.fittedCount = fitted.size();
        auto provisional = input.tempi;
        provisional.erase(provisional.lowerBound(plan.begin), provisional.end());
        for (auto it = fitted.cbegin(); it != fitted.cend(); ++it)
            provisional.insert(it.key(), it.value());
        const TempoMapping provisionalMapping(provisional, input.resolution);
        const auto startSeconds = plan.moveAudio ? provisionalMapping.seconds(firstTick) - firstSeconds : input.startSeconds;
        const auto shift = startSeconds - input.startSeconds;
        if (!std::isfinite(startSeconds) || startSeconds < 0)
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "Choose a later reference position."));
        if ((startSeconds + input.durationSeconds) * 1000 > std::numeric_limits<int>::max())
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The extracted tempo exceeds the project time range."));
        const auto endTick = std::ceil(provisionalMapping.tick(startSeconds + input.durationSeconds) - 1e-8);
        if (!std::isfinite(endTick) || endTick <= plan.begin || endTick >= std::numeric_limits<int>::max())
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The extracted tempo exceeds the project time range."));
        plan.end = static_cast<int>(endTick);
        plan.tempi = input.tempi;
        plan.tempi.erase(plan.tempi.lowerBound(plan.begin), plan.tempi.lowerBound(plan.end));
        for (auto it = fitted.cbegin(); it != fitted.cend() && it.key() < plan.end; ++it)
            plan.tempi.insert(it.key(), it.value());
        const auto restore = tempoAt(input.tempi, plan.end);
        if (!plan.tempi.contains(plan.end) && tempoAt(plan.tempi, plan.end) != restore)
            plan.tempi.insert(plan.end, restore);

        const TempoMapping mapping(plan.tempi, input.resolution);
        const auto oldOrigin = oldMapping.seconds(input.position - static_cast<double>(input.clipStart));
        const auto contentEnd = oldMapping.seconds(input.position - static_cast<double>(input.clipStart) + input.length);
        const auto position = std::round(mapping.tick(startSeconds));
        const auto newOrigin = std::round(mapping.tick(oldOrigin + shift));
        const auto newLength = std::round(mapping.tick(contentEnd + shift)) - newOrigin;
        const auto newClipStart = position - newOrigin;
        const auto newClipLength = std::round(mapping.tick(startSeconds + input.durationSeconds)) - position;
        if (position < 0 || position >= std::numeric_limits<int>::max() || newClipStart < 0 || newClipStart > std::numeric_limits<int>::max()
            || newLength < 0 || newLength > std::numeric_limits<int>::max() || newClipLength <= 0 || newClipLength > std::numeric_limits<int>::max())
            return fail(QT_TRANSLATE_NOOP("Synth::Internal::TempoExtractionPreview", "The audio region cannot be preserved with this tempo."));
        plan.position = static_cast<int>(position);
        plan.clipStart = static_cast<int>(newClipStart);
        plan.clipLength = static_cast<int>(newClipLength);
        plan.length = static_cast<int>(newLength);
        plan.audioShiftMsec = shift * 1000;
        for (auto it = fitted.cbegin(); it != fitted.cend(); ++it) {
            if (it.key() >= plan.end)
                break;
            const auto seconds = std::max(startSeconds, mapping.seconds(it.key()));
            if (!plan.tempoPoints.isEmpty())
                plan.tempoPoints.append({seconds, plan.tempoPoints.last().y()});
            plan.tempoPoints.append({seconds, it.value()});
        }
        plan.tempoPoints.append({startSeconds + input.durationSeconds, plan.tempoPoints.last().y()});
        for (qsizetype i = 0; i < input.beats.size(); ++i) {
            if (options.variable && !grid.usable[i])
                continue;
            qint64 tick = plan.beatTicks[i];
            if (!options.variable) {
                const auto nearest = firstPosition + std::round((input.beats[i].seconds - firstSeconds) / fixedPeriod);
                tick = originTick + qRound64(nearest * quarterTicks);
            }
            const auto seconds = startSeconds + input.beats.at(i).seconds;
            const auto error = (mapping.seconds(tick) - seconds) * 1000;
            plan.errors.append({seconds, error});
            plan.maxErrorMsec = std::max(plan.maxErrorMsec, std::abs(error));
        }
        auto displayInput = input;
        displayInput.startSeconds = startSeconds;
        plan.candidateBeats = beatLines(displayInput, plan.tempi, cancelled);
        return plan;
    }

}
