#ifndef DIFFSCOPE_AUDIO_WAVEFORMSINGERMODEL_H
#define DIFFSCOPE_AUDIO_WAVEFORMSINGERMODEL_H

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <opendspx/anchornode.h>
#include <opendspx/vibrato.h>
#include <opendspxinterpolator/parameterinterpolator.h>
#include <opendspxinterpolator/vibratocurve.h>

namespace Audio::Internal {

    inline constexpr int waveformSingerTypeCount = 3;

    struct WaveformSingerTempoSegment {
        double tick{};
        double seconds{};
        double beatsPerMinute{120.0};
    };

    struct WaveformSingerTempoSnapshot {
        std::uint64_t revision{};
        int ticksPerQuarter{480};
        std::vector<WaveformSingerTempoSegment> segments;

        double tickToSeconds(double tick) const;
        double secondsToTick(double seconds) const;
    };

    struct WaveformSingerFreeCurve {
        static constexpr int step = 5;
        static constexpr int blockSize = 512;

        struct Block {
            std::array<int, blockSize> values{};
            std::array<std::uint64_t, blockSize / 64> valid{};
        };

        int size{};
        std::vector<std::shared_ptr<const Block>> blocks;

        std::optional<double> evaluate(double tick) const;
    };

    struct WaveformSingerAnchorCurve {
        struct Segment {
            int firstTick{};
            int lastTick{};
            std::shared_ptr<const opendspx::ParameterInterpolator> interpolator;
        };

        std::vector<Segment> segments;

        std::optional<double> evaluate(double tick) const;
    };

    struct WaveformSingerParameterSnapshot {
        WaveformSingerFreeCurve original;
        WaveformSingerFreeCurve freeEdited;
        WaveformSingerAnchorCurve anchorEdited;
        WaveformSingerFreeCurve freeTransform;
        WaveformSingerAnchorCurve anchorTransform;

        std::optional<double> evaluate(double tick) const;
    };

    struct WaveformSingerMixAnchor {
        int tick{};
        std::vector<double> weights;
    };

    struct WaveformSingerVoiceSnapshot {
        std::vector<std::array<double, waveformSingerTypeCount>> roots;
        std::vector<WaveformSingerMixAnchor> anchors;

        void evaluate(double tick, std::array<double, waveformSingerTypeCount> &result) const;
    };

    struct WaveformSingerClipSnapshot {
        std::uint64_t revision{};
        std::uint64_t phaseRevision{};
        int startTick{};
        int positionTick{};
        int contentLengthTick{};
        int clipStartTick{};
        int clipLengthTick{};
        std::shared_ptr<const WaveformSingerParameterSnapshot> pitch;
        std::shared_ptr<const WaveformSingerParameterSnapshot> energy;
        std::shared_ptr<const WaveformSingerParameterSnapshot> toneShift;
        std::shared_ptr<const WaveformSingerVoiceSnapshot> voices;
    };

    struct WaveformSingerNoteSnapshot {
        std::uint64_t revision{};
        int positionTick{};
        int lengthTick{};
        int keyNumber{};
        int centShift{};
        std::uint64_t seed{};
        std::shared_ptr<const opendspx::VibratoCurve> vibrato;
    };

    template<class Snapshot>
    class WaveformSingerAtomicModel {
    public:
        std::shared_ptr<const Snapshot> snapshot() const {
            return std::atomic_load_explicit(&m_snapshot, std::memory_order_acquire);
        }

        void publish(std::shared_ptr<const Snapshot> snapshot) {
            std::atomic_store_explicit(&m_snapshot, std::move(snapshot), std::memory_order_release);
        }

    private:
        std::shared_ptr<const Snapshot> m_snapshot;
    };

    using WaveformSingerTempoModel = WaveformSingerAtomicModel<WaveformSingerTempoSnapshot>;
    using WaveformSingerClipModel = WaveformSingerAtomicModel<WaveformSingerClipSnapshot>;
    using WaveformSingerNoteModel = WaveformSingerAtomicModel<WaveformSingerNoteSnapshot>;

}

#endif // DIFFSCOPE_AUDIO_WAVEFORMSINGERMODEL_H
