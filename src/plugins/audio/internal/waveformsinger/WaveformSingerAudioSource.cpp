#include "WaveformSingerAudioSource.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <TalcsCore/IAudioSampleContainer.h>

#include <audio/internal/WaveformSingerSynthesizer.h>

namespace Audio::Internal {

    namespace {

        constexpr qint64 phaseCheckpointStep = 4096;

        qint64 sampleAt(double seconds, double sampleRate) {
            return static_cast<qint64>(std::llround(seconds * sampleRate));
        }

        double frequencyFromCents(double cents) {
            return 440.0 * std::pow(2.0, (cents / 100.0 - 69.0) / 12.0);
        }

        std::size_t tempoSegmentAt(const WaveformSingerTempoSnapshot &tempo, double seconds) {
            const auto right = std::upper_bound(
                tempo.segments.begin(), tempo.segments.end(), seconds,
                [](double value, const WaveformSingerTempoSegment &segment) {
                    return value < segment.seconds;
                });
            return right == tempo.segments.begin()
                ? 0
                : static_cast<std::size_t>(std::distance(tempo.segments.begin(), right) - 1);
        }

        double tickAtSeconds(const WaveformSingerTempoSnapshot &tempo, double seconds,
                             std::size_t &segmentIndex) {
            while (segmentIndex + 1 < tempo.segments.size() &&
                   seconds >= tempo.segments[segmentIndex + 1].seconds) {
                ++segmentIndex;
            }
            while (segmentIndex > 0 && seconds < tempo.segments[segmentIndex].seconds) {
                --segmentIndex;
            }
            const auto &segment = tempo.segments[segmentIndex];
            return segment.tick + (seconds - segment.seconds) *
                   segment.beatsPerMinute * tempo.ticksPerQuarter / 60.0;
        }

    }

    WaveformSingerNoteAudioSource::WaveformSingerNoteAudioSource(
        std::shared_ptr<WaveformSingerTempoModel> tempoModel,
        std::shared_ptr<WaveformSingerClipModel> clipModel,
        std::shared_ptr<WaveformSingerNoteModel> noteModel)
        : m_tempoModel(std::move(tempoModel)),
          m_clipModel(std::move(clipModel)),
          m_noteModel(std::move(noteModel)) {
    }

    WaveformSingerNoteAudioSource::~WaveformSingerNoteAudioSource() {
        WaveformSingerNoteAudioSource::close();
    }

    bool WaveformSingerNoteAudioSource::open(qint64 bufferSize, double sampleRate) {
        close();
        if (!talcs::AudioSource::open(bufferSize, sampleRate)) {
            return false;
        }
        const auto checkpointCount = std::max<qint64>(1, calculatedLength(sampleRate) / phaseCheckpointStep + 2);
        m_checkpoints.assign(static_cast<std::size_t>(checkpointCount), {});
        m_position = 0;
        m_cycles = 0.0;
        m_revision = 0;
        m_phaseDirty = true;
        m_tempoSegment = 0;
        return true;
    }

    void WaveformSingerNoteAudioSource::close() {
        m_checkpoints.clear();
        m_checkpoints.shrink_to_fit();
        m_position = 0;
        m_cycles = 0.0;
        m_revision = 0;
        m_phaseDirty = true;
        m_tempoSegment = 0;
        talcs::AudioSource::close();
    }

    qint64 WaveformSingerNoteAudioSource::length() const {
        return isOpen() ? calculatedLength(sampleRate()) : std::numeric_limits<qint64>::max();
    }

    qint64 WaveformSingerNoteAudioSource::nextReadPosition() const {
        return m_position;
    }

    void WaveformSingerNoteAudioSource::setNextReadPosition(qint64 position) {
        const auto target = std::max<qint64>(0, position);
        m_phaseDirty = m_phaseDirty || target != m_position;
        m_position = target;
    }

    std::shared_ptr<WaveformSingerNoteModel> WaveformSingerNoteAudioSource::noteModel() const {
        return m_noteModel;
    }

    qint64 WaveformSingerNoteAudioSource::processReading(const talcs::AudioSourceReadData &readData) {
        for (int channel = 0; channel < readData.buffer->channelCount(); ++channel) {
            readData.buffer->clear(channel, readData.startPos, readData.length);
        }
        const auto tempo = m_tempoModel->snapshot();
        const auto clip = m_clipModel->snapshot();
        const auto note = m_noteModel->snapshot();
        if (!tempo || !clip || !note || qFuzzyIsNull(sampleRate())) {
            m_position += readData.length;
            m_phaseDirty = true;
            return readData.length;
        }

        const auto revision = currentRevision(*tempo, *clip, *note);
        const auto evaluation = evaluationContext(*tempo, *clip, *note);
        if (revision != m_revision || m_phaseDirty) {
            restorePhase(m_position, revision, evaluation, *tempo, *clip, *note);
        } else if (m_position == 0) {
            m_cycles = 0.0;
        }

        for (qint64 offset = 0; offset < readData.length; ++offset) {
            if (m_position % phaseCheckpointStep == 0) {
                const auto checkpointIndex = static_cast<std::size_t>(m_position / phaseCheckpointStep);
                if (checkpointIndex < m_checkpoints.size()) {
                    m_checkpoints[checkpointIndex] = {revision, m_cycles};
                }
            }
            const auto control = controlAt(m_position, m_tempoSegment, evaluation, *tempo, *clip, *note);
            if (readData.silentFlags != -1) {
                const auto sample = WaveformSingerSynthesizer::render(
                    m_cycles, control.frequency, control.noteTime, control.noteOffTime,
                    note->seed, control.weights, sampleRate()) * static_cast<float>(control.energyGain);
                for (int channel = 0; channel < readData.buffer->channelCount(); ++channel) {
                    if ((readData.silentFlags & (1 << channel)) == 0) {
                        readData.buffer->setSample(channel, readData.startPos + offset, sample);
                    }
                }
            }
            m_cycles += control.frequency / sampleRate();
            ++m_position;
        }
        m_revision = revision;
        m_phaseDirty = false;
        return readData.length;
    }

    std::uint64_t WaveformSingerNoteAudioSource::currentRevision(
        const WaveformSingerTempoSnapshot &tempo, const WaveformSingerClipSnapshot &clip,
        const WaveformSingerNoteSnapshot &note) const {
        auto revision = tempo.revision;
        revision ^= clip.phaseRevision + 0x9e3779b97f4a7c15ULL + (revision << 6) + (revision >> 2);
        revision ^= note.revision + 0x9e3779b97f4a7c15ULL + (revision << 6) + (revision >> 2);
        return revision == 0 ? 1 : revision;
    }

    WaveformSingerNoteAudioSource::EvaluationContext WaveformSingerNoteAudioSource::evaluationContext(
        const WaveformSingerTempoSnapshot &tempo, const WaveformSingerClipSnapshot &clip,
        const WaveformSingerNoteSnapshot &note) const {
        EvaluationContext result;
        result.noteStartTick = clip.startTick + note.positionTick;
        result.noteEndTick = result.noteStartTick + note.lengthTick;
        result.noteStartSeconds = tempo.tickToSeconds(result.noteStartTick);
        result.noteEndSeconds = tempo.tickToSeconds(result.noteEndTick);
        result.noteDuration = std::max(0.0, result.noteEndSeconds - result.noteStartSeconds);
        return result;
    }

    WaveformSingerNoteAudioSource::ControlValue WaveformSingerNoteAudioSource::controlAt(
        qint64 samplePosition, std::size_t &tempoSegment, const EvaluationContext &evaluation,
        const WaveformSingerTempoSnapshot &tempo,
        const WaveformSingerClipSnapshot &clip, const WaveformSingerNoteSnapshot &note) const {
        ControlValue result;
        const double seconds = evaluation.noteStartSeconds + static_cast<double>(samplePosition) / sampleRate();
        const double evaluationTick = std::min(
            evaluation.noteEndTick, tickAtSeconds(tempo, seconds, tempoSegment));
        const double parameterTick = evaluationTick - clip.startTick;
        result.noteTime = std::max(0.0, seconds - evaluation.noteStartSeconds);
        result.noteOffTime = evaluation.noteDuration;

        const auto pitchValue = clip.pitch ? clip.pitch->evaluate(parameterTick) : std::nullopt;
        const auto toneShiftValue = clip.toneShift ? clip.toneShift->evaluate(parameterTick) : std::nullopt;
        const auto energyValue = clip.energy ? clip.energy->evaluate(parameterTick) : std::nullopt;
        const double normalizedNoteTime = evaluation.noteDuration > 0.0
            ? std::clamp(result.noteTime / evaluation.noteDuration, 0.0, 1.0)
            : 0.0;
        const double vibrato = note.vibrato ? note.vibrato->evaluate(normalizedNoteTime) : 0.0;
        const double basePitch = pitchValue.value_or(note.keyNumber * 100.0 + note.centShift);
        result.frequency = frequencyFromCents(basePitch + toneShiftValue.value_or(0.0) + vibrato);
        result.energyGain = std::pow(10.0, energyValue.value_or(0.0) / 20000.0);
        if (clip.voices) {
            clip.voices->evaluate(parameterTick, result.weights);
        } else {
            result.weights.fill(0.0);
        }
        return result;
    }

    void WaveformSingerNoteAudioSource::restorePhase(
        qint64 target, std::uint64_t revision, const EvaluationContext &evaluation,
        const WaveformSingerTempoSnapshot &tempo,
        const WaveformSingerClipSnapshot &clip, const WaveformSingerNoteSnapshot &note) {
        qint64 cursor = 0;
        m_cycles = 0.0;
        auto checkpointIndex = std::min<std::size_t>(
            static_cast<std::size_t>(target / phaseCheckpointStep),
            m_checkpoints.empty() ? 0 : m_checkpoints.size() - 1);
        while (!m_checkpoints.empty()) {
            const auto &checkpoint = m_checkpoints[checkpointIndex];
            if (checkpoint.revision == revision) {
                cursor = static_cast<qint64>(checkpointIndex) * phaseCheckpointStep;
                m_cycles = checkpoint.cycles;
                break;
            }
            if (checkpointIndex == 0) {
                break;
            }
            --checkpointIndex;
        }
        if (!m_checkpoints.empty()) {
            m_checkpoints[0] = {revision, 0.0};
        }
        m_tempoSegment = tempoSegmentAt(
            tempo, evaluation.noteStartSeconds + static_cast<double>(cursor) / sampleRate());
        while (cursor < target) {
            const auto control = controlAt(cursor, m_tempoSegment, evaluation, tempo, clip, note);
            m_cycles += control.frequency / sampleRate();
            ++cursor;
            if (cursor % phaseCheckpointStep == 0) {
                const auto index = static_cast<std::size_t>(cursor / phaseCheckpointStep);
                if (index < m_checkpoints.size()) {
                    m_checkpoints[index] = {revision, m_cycles};
                }
            }
        }
        m_revision = revision;
        m_phaseDirty = false;
    }

    qint64 WaveformSingerNoteAudioSource::calculatedLength(double targetSampleRate) const {
        const auto tempo = m_tempoModel->snapshot();
        const auto clip = m_clipModel->snapshot();
        const auto note = m_noteModel->snapshot();
        if (!tempo || !clip || !note || qFuzzyIsNull(targetSampleRate)) {
            return 1;
        }
        const auto start = tempo->tickToSeconds(clip->startTick + note->positionTick);
        const auto end = tempo->tickToSeconds(clip->startTick + note->positionTick + note->lengthTick);
        return std::max<qint64>(1, sampleAt(end - start + WaveformSingerSynthesizer::maximumReleaseSeconds(), targetSampleRate));
    }

    WaveformSingerClipAudioSource::WaveformSingerClipAudioSource(
        std::shared_ptr<WaveformSingerTempoModel> tempoModel,
        std::shared_ptr<WaveformSingerClipModel> clipModel)
        : m_tempoModel(std::move(tempoModel)),
          m_clipModel(std::move(clipModel)),
          m_controlMixer(std::make_unique<talcs::PositionableMixerAudioSource>()),
          m_noteSeries(std::make_unique<talcs::AudioSourceClipSeries>()) {
        m_controlMixer->addSource(m_noteSeries.get());
    }

    WaveformSingerClipAudioSource::~WaveformSingerClipAudioSource() {
        m_noteSeries->removeAllClips();
        m_notes.clear();
        m_controlMixer->removeSource(m_noteSeries.get());
    }

    talcs::PositionableMixerAudioSource *WaveformSingerClipAudioSource::controlMixer() {
        return m_controlMixer.get();
    }

    std::shared_ptr<WaveformSingerClipModel> WaveformSingerClipAudioSource::clipModel() const {
        return m_clipModel;
    }

    WaveformSingerNoteAudioSource *WaveformSingerClipAudioSource::addNote(
        std::shared_ptr<WaveformSingerNoteModel> noteModel) {
        NoteEntry entry;
        entry.source = std::make_unique<WaveformSingerNoteAudioSource>(m_tempoModel, m_clipModel, std::move(noteModel));
        entry.view = m_noteSeries->insertClip(entry.source.get(), 0, 0, 1);
        const auto result = entry.source.get();
        m_noteViews.emplace(result, entry.view);
        m_notes.push_back(std::move(entry));
        return result;
    }

    void WaveformSingerClipAudioSource::removeNote(WaveformSingerNoteAudioSource *note) {
        const auto it = std::find_if(m_notes.begin(), m_notes.end(), [note](const NoteEntry &entry) {
            return entry.source.get() == note;
        });
        if (it == m_notes.end()) {
            return;
        }
        m_noteSeries->removeClip(m_noteViews.at(note));
        m_noteViews.erase(note);
        m_notes.erase(it);
    }

    void WaveformSingerClipAudioSource::refreshNoteRange(
        WaveformSingerNoteAudioSource *noteSource, double targetSampleRate) {
        const auto tempo = m_tempoModel->snapshot();
        const auto clip = m_clipModel->snapshot();
        const auto note = noteSource ? noteSource->noteModel()->snapshot() : nullptr;
        const auto view = m_noteViews.find(noteSource);
        if (!tempo || !clip || !note || view == m_noteViews.end() || qFuzzyIsNull(targetSampleRate)) {
            return;
        }
        const auto contentOriginSeconds = tempo->tickToSeconds(clip->startTick);
        const auto noteStartSeconds = tempo->tickToSeconds(clip->startTick + note->positionTick);
        const auto noteEndSeconds = tempo->tickToSeconds(
            clip->startTick + note->positionTick + note->lengthTick);
        m_noteSeries->setClipRange(
            view->second,
            sampleAt(noteStartSeconds - contentOriginSeconds, targetSampleRate),
            std::max<qint64>(1, sampleAt(noteEndSeconds - noteStartSeconds +
                                        WaveformSingerSynthesizer::maximumReleaseSeconds(), targetSampleRate)));
    }

    void WaveformSingerClipAudioSource::refreshRanges(
        talcs::AudioSourceClipSeries *trackSeries, double targetSampleRate) {
        const auto tempo = m_tempoModel->snapshot();
        const auto clip = m_clipModel->snapshot();
        if (!tempo || !clip || qFuzzyIsNull(targetSampleRate)) {
            return;
        }
        const auto visibleLength = clip->contentLengthTick > 0
            ? std::min(clip->clipLengthTick, std::max(0, clip->contentLengthTick - clip->clipStartTick))
            : clip->clipLengthTick;
        const auto contentOriginSeconds = tempo->tickToSeconds(clip->startTick);
        const auto positionSeconds = tempo->tickToSeconds(clip->positionTick);
        trackSeries->setClipStartPos(m_trackView, sampleAt(positionSeconds - contentOriginSeconds, targetSampleRate));
        trackSeries->setClipRange(
            m_trackView,
            sampleAt(positionSeconds, targetSampleRate),
            std::max<qint64>(1, sampleAt(tempo->tickToSeconds(clip->positionTick + visibleLength) - positionSeconds,
                                        targetSampleRate)));

        for (auto &entry : m_notes) {
            refreshNoteRange(entry.source.get(), targetSampleRate);
        }
    }

    WaveformSingerTrackAudioSource::WaveformSingerTrackAudioSource(
        std::shared_ptr<WaveformSingerTempoModel> tempoModel)
        : m_tempoModel(std::move(tempoModel)) {
    }

    WaveformSingerTrackAudioSource::~WaveformSingerTrackAudioSource() {
        removeAllClips();
        m_clips.clear();
    }

    WaveformSingerClipAudioSource *WaveformSingerTrackAudioSource::addClip(
        std::shared_ptr<WaveformSingerClipModel> clipModel) {
        auto clip = std::make_unique<WaveformSingerClipAudioSource>(m_tempoModel, std::move(clipModel));
        clip->m_trackView = insertClip(clip->controlMixer(), 0, 0, 1);
        const auto result = clip.get();
        m_clips.push_back(std::move(clip));
        if (isOpen()) {
            result->refreshRanges(this, sampleRate());
        }
        return result;
    }

    void WaveformSingerTrackAudioSource::removeClip(WaveformSingerClipAudioSource *clip) {
        const auto it = std::find_if(m_clips.begin(), m_clips.end(), [clip](const auto &item) {
            return item.get() == clip;
        });
        if (it == m_clips.end()) {
            return;
        }
        talcs::AudioSourceClipSeries::removeClip((*it)->m_trackView);
        m_clips.erase(it);
    }

    void WaveformSingerTrackAudioSource::refreshRanges(double targetSampleRate) {
        for (const auto &clip : m_clips) {
            clip->refreshRanges(this, targetSampleRate);
        }
    }

    bool WaveformSingerTrackAudioSource::open(qint64 bufferSize, double targetSampleRate) {
        refreshRanges(targetSampleRate);
        return talcs::AudioSourceClipSeries::open(bufferSize, targetSampleRate);
    }

}
