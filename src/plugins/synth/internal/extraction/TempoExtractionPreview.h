// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_TEMPOEXTRACTIONPREVIEW_H
#define DIFFSCOPE_SYNTH_TEMPOEXTRACTIONPREVIEW_H

#include <atomic>
#include <memory>

#include <QObject>
#include <QTimer>
#include <QValidator>
#include <QVariantList>
#include <qqmlintegration.h>

#include <SVSCraftCore/WaveformMipmap.h>

#include <synth/internal/ExtractionAudioCodec.h>
#include <synth/internal/TempoExtractionPlanner.h>

namespace SVS {
    class LongTimeValidator;
    class MusicTimeline;
}

namespace talcs {
    class TransportAudioSource;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace Synth::Internal {

    class TempoExtractionAudioSource;

    class TempoExtractionPreview : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(bool busy READ busy NOTIFY changed)
        Q_PROPERTY(bool valid READ valid NOTIFY changed)
        Q_PROPERTY(QString error READ error NOTIFY changed)
        Q_PROPERTY(int tempoChangeCount READ tempoChangeCount NOTIFY changed)
        Q_PROPERTY(double maximumOffsetMsec READ maximumOffsetMsec NOTIFY changed)
        Q_PROPERTY(bool canKeepAudioPosition READ canKeepAudioPosition NOTIFY changed)
        Q_PROPERTY(QString alignmentHint READ alignmentHint NOTIFY changed)
        Q_PROPERTY(bool variable READ variable WRITE setVariable NOTIFY optionsChanged)
        Q_PROPERTY(bool automatic READ automatic WRITE setAutomatic NOTIFY optionsChanged)
        Q_PROPERTY(double bpm READ bpm WRITE setBpm NOTIFY optionsChanged)
        Q_PROPERTY(double smoothing READ smoothing WRITE setSmoothing NOTIFY optionsChanged)
        Q_PROPERTY(int minimumSegmentBeats READ minimumSegmentBeats WRITE setMinimumSegmentBeats NOTIFY optionsChanged)
        Q_PROPERTY(bool moveAudio READ moveAudio WRITE setMoveAudio NOTIFY optionsChanged)
        Q_PROPERTY(int referenceBeat READ referenceBeat WRITE setReferenceBeat NOTIFY optionsChanged)
        Q_PROPERTY(int referenceTick READ referenceTick WRITE setReferenceTick NOTIFY optionsChanged)
        Q_PROPERTY(QVariantList beatLabels READ beatLabels NOTIFY timeOriginChanged)
        Q_PROPERTY(SVS::MusicTimeline *timeline READ timeline CONSTANT)
        Q_PROPERTY(double begin READ begin NOTIFY timeOriginChanged)
        Q_PROPERTY(double end READ end NOTIFY timeOriginChanged)
        Q_PROPERTY(double viewStart READ viewStart NOTIFY rangeChanged)
        Q_PROPERTY(double viewEnd READ viewEnd NOTIFY rangeChanged)
        Q_PROPERTY(QValidator *startTimeValidator READ startTimeValidator CONSTANT)
        Q_PROPERTY(QValidator *endTimeValidator READ endTimeValidator CONSTANT)
        Q_PROPERTY(QValidator *timeValidator READ timeValidator CONSTANT)
        Q_PROPERTY(SVS::WaveformMipmap waveform READ waveform NOTIFY waveformChanged)
        Q_PROPERTY(double waveformRate READ waveformRate NOTIFY waveformChanged)
        Q_PROPERTY(bool playing READ playing NOTIFY playbackChanged)
        Q_PROPERTY(double playhead READ playhead NOTIFY playbackChanged)
        Q_PROPERTY(bool metronome READ metronome WRITE setMetronome NOTIFY playbackChanged)
        Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY playbackChanged)
    public:
        explicit TempoExtractionPreview(const TempoExtractionInput &input, const ExtractionAudioCodec::Input &audio, Core::ProjectWindowInterface *window, QObject *parent = nullptr);
        ~TempoExtractionPreview() override;

        bool busy() const;
        bool valid() const;
        QString error() const;
        int tempoChangeCount() const;
        double maximumOffsetMsec() const;
        bool canKeepAudioPosition() const;
        QString alignmentHint() const;
        bool variable() const;
        void setVariable(bool value);
        bool automatic() const;
        void setAutomatic(bool value);
        double bpm() const;
        void setBpm(double value);
        double smoothing() const;
        void setSmoothing(double value);
        int minimumSegmentBeats() const;
        void setMinimumSegmentBeats(int value);
        bool moveAudio() const;
        void setMoveAudio(bool value);
        int referenceBeat() const;
        void setReferenceBeat(int value);
        int referenceTick() const;
        void setReferenceTick(int value);
        QVariantList beatLabels() const;
        SVS::MusicTimeline *timeline() const;
        double begin() const;
        double end() const;
        double viewStart() const;
        double viewEnd() const;
        QValidator *startTimeValidator() const;
        QValidator *endTimeValidator() const;
        QValidator *timeValidator() const;
        SVS::WaveformMipmap waveform() const;
        double waveformRate() const;
        bool playing() const;
        double playhead() const;
        bool metronome() const;
        void setMetronome(bool value);
        bool loop() const;
        void setLoop(bool value);
        const TempoExtractionInput &input() const;
        const TempoExtractionPlan &plan() const;

        Q_INVOKABLE void setRange(double start, double end);
        Q_INVOKABLE QString formatTime(int msec) const;
        Q_INVOKABLE int parseTime(const QString &text, int fallback) const;
        Q_INVOKABLE double tempoAt(double seconds) const;
        Q_INVOKABLE void scaleBpm(double factor);
        Q_INVOKABLE void togglePlayback();
        Q_INVOKABLE void pause();
        Q_INVOKABLE void seek(double seconds);
        Q_INVOKABLE void selectBeat(double seconds, double radius);

    Q_SIGNALS:
        void changed();
        void optionsChanged();
        void timeOriginChanged();
        void rangeChanged();
        void waveformChanged();
        void playbackChanged();

    private:
        void schedule();
        void calculate();
        void loadWaveform();
        void releasePlayback();

        TempoExtractionInput m_input;
        ExtractionAudioCodec::Input m_audio;
        Core::ProjectWindowInterface *m_window{};
        TempoExtractionOptions m_options;
        TempoExtractionPlan m_plan;
        SVS::MusicTimeline *m_timeline{};
        SVS::LongTimeValidator *m_startValidator{};
        SVS::LongTimeValidator *m_endValidator{};
        SVS::LongTimeValidator *m_timeValidator{};
        SVS::WaveformMipmap m_waveform;
        double m_waveformRate{};
        QString m_audioError;
        QTimer m_calculationTimer;
        QTimer m_playbackTimer;
        std::shared_ptr<std::atomic_bool> m_cancelled;
        std::shared_ptr<std::atomic_bool> m_waveformCancelled;
        quint64 m_revision{};
        double m_viewStart{};
        double m_viewEnd{};
        double m_playhead{};
        double m_outputRate{};
        bool m_busy{true};
        bool m_playing{};
        bool m_metronome{true};
        bool m_loop{};
        std::unique_ptr<TempoExtractionAudioSource> m_source;
        std::unique_ptr<talcs::TransportAudioSource> m_transport;
    };

}

#endif // DIFFSCOPE_SYNTH_TEMPOEXTRACTIONPREVIEW_H
