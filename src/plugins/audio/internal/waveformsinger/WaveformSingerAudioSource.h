#ifndef DIFFSCOPE_AUDIO_WAVEFORMSINGERAUDIOSOURCE_H
#define DIFFSCOPE_AUDIO_WAVEFORMSINGERAUDIOSOURCE_H

#include <memory>
#include <unordered_map>
#include <vector>

#include <TalcsCore/AudioSourceClipSeries.h>
#include <TalcsCore/PositionableAudioSource.h>
#include <TalcsCore/PositionableMixerAudioSource.h>

#include <audio/internal/WaveformSingerModel.h>

namespace Audio::Internal {

    class WaveformSingerNoteAudioSource final : public talcs::PositionableAudioSource {
    public:
        WaveformSingerNoteAudioSource(std::shared_ptr<WaveformSingerTempoModel> tempoModel,
                                      std::shared_ptr<WaveformSingerClipModel> clipModel,
                                      std::shared_ptr<WaveformSingerNoteModel> noteModel);
        ~WaveformSingerNoteAudioSource() override;

        bool open(qint64 bufferSize, double sampleRate) override;
        void close() override;
        qint64 length() const override;
        qint64 nextReadPosition() const override;
        void setNextReadPosition(qint64 position) override;

        std::shared_ptr<WaveformSingerNoteModel> noteModel() const;

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override;

    private:
        struct Checkpoint {
            std::uint64_t revision{};
            double cycles{};
        };

        struct ControlValue {
            double frequency{};
            double energyGain{1.0};
            std::array<double, waveformSingerTypeCount> weights{};
            double noteTime{};
            double noteOffTime{};
        };

        struct EvaluationContext {
            double noteStartTick{};
            double noteEndTick{};
            double noteStartSeconds{};
            double noteEndSeconds{};
            double noteDuration{};
        };

        std::uint64_t currentRevision(const WaveformSingerTempoSnapshot &tempo,
                                      const WaveformSingerClipSnapshot &clip,
                                      const WaveformSingerNoteSnapshot &note) const;
        EvaluationContext evaluationContext(const WaveformSingerTempoSnapshot &tempo,
                                            const WaveformSingerClipSnapshot &clip,
                                            const WaveformSingerNoteSnapshot &note) const;
        ControlValue controlAt(qint64 samplePosition, std::size_t &tempoSegment,
                               const EvaluationContext &evaluation,
                               const WaveformSingerTempoSnapshot &tempo,
                               const WaveformSingerClipSnapshot &clip,
                               const WaveformSingerNoteSnapshot &note) const;
        void restorePhase(qint64 target, std::uint64_t revision,
                          const EvaluationContext &evaluation,
                          const WaveformSingerTempoSnapshot &tempo,
                          const WaveformSingerClipSnapshot &clip,
                          const WaveformSingerNoteSnapshot &note);
        qint64 calculatedLength(double sampleRate) const;

        std::shared_ptr<WaveformSingerTempoModel> m_tempoModel;
        std::shared_ptr<WaveformSingerClipModel> m_clipModel;
        std::shared_ptr<WaveformSingerNoteModel> m_noteModel;
        std::vector<Checkpoint> m_checkpoints;
        qint64 m_position{};
        double m_cycles{};
        std::uint64_t m_revision{};
        bool m_phaseDirty{true};
        std::size_t m_tempoSegment{};
    };

    class WaveformSingerClipAudioSource final {
    public:
        WaveformSingerClipAudioSource(std::shared_ptr<WaveformSingerTempoModel> tempoModel,
                                      std::shared_ptr<WaveformSingerClipModel> clipModel);
        ~WaveformSingerClipAudioSource();

        talcs::PositionableMixerAudioSource *controlMixer();
        std::shared_ptr<WaveformSingerClipModel> clipModel() const;

        WaveformSingerNoteAudioSource *addNote(std::shared_ptr<WaveformSingerNoteModel> noteModel);
        void removeNote(WaveformSingerNoteAudioSource *note);
        void refreshNoteRange(WaveformSingerNoteAudioSource *note, double sampleRate);
        void refreshRanges(talcs::AudioSourceClipSeries *trackSeries, double sampleRate);

    private:
        struct NoteEntry {
            std::unique_ptr<WaveformSingerNoteAudioSource> source;
            talcs::AudioSourceClipSeries::ClipView view;
        };

        std::shared_ptr<WaveformSingerTempoModel> m_tempoModel;
        std::shared_ptr<WaveformSingerClipModel> m_clipModel;
        std::unique_ptr<talcs::PositionableMixerAudioSource> m_controlMixer;
        std::unique_ptr<talcs::AudioSourceClipSeries> m_noteSeries;
        std::vector<NoteEntry> m_notes;
        std::unordered_map<WaveformSingerNoteAudioSource *, talcs::AudioSourceClipSeries::ClipView> m_noteViews;
        talcs::AudioSourceClipSeries::ClipView m_trackView;

        friend class WaveformSingerTrackAudioSource;
    };

    class WaveformSingerTrackAudioSource final : public talcs::AudioSourceClipSeries {
    public:
        explicit WaveformSingerTrackAudioSource(std::shared_ptr<WaveformSingerTempoModel> tempoModel);
        ~WaveformSingerTrackAudioSource() override;

        WaveformSingerClipAudioSource *addClip(std::shared_ptr<WaveformSingerClipModel> clipModel);
        void removeClip(WaveformSingerClipAudioSource *clip);
        void refreshRanges(double sampleRate);
        bool open(qint64 bufferSize, double sampleRate) override;

    private:
        std::shared_ptr<WaveformSingerTempoModel> m_tempoModel;
        std::vector<std::unique_ptr<WaveformSingerClipAudioSource>> m_clips;
    };

}

#endif // DIFFSCOPE_AUDIO_WAVEFORMSINGERAUDIOSOURCE_H
