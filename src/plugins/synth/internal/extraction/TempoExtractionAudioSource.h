// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_TEMPOEXTRACTIONAUDIOSOURCE_H
#define DIFFSCOPE_SYNTH_TEMPOEXTRACTIONAUDIOSOURCE_H

#include <QPointF>
#include <QVector>

#include <TalcsCore/AudioBuffer.h>
#include <TalcsCore/MetronomeAudioSource.h>
#include <TalcsCore/PositionableAudioSource.h>
#include <TalcsFormat/AudioFormatInputSource.h>

namespace Synth::Internal {

    class TempoExtractionAudioSource : public talcs::PositionableAudioSource, private talcs::MetronomeAudioSourceDetector {
    public:
        explicit TempoExtractionAudioSource(talcs::AbstractAudioFormatIO *io, double offsetSeconds, double durationSeconds);
        bool open(qint64 bufferSize, double sampleRate) override;
        void close() override;
        qint64 length() const override;
        void setNextReadPosition(qint64 position) override;
        void setBeats(const QVector<QPointF> &beats, double startSeconds);
        void setMetronomeEnabled(bool enabled);
        void setPlaybackEnd(double seconds);

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override;

    private:
        void detectInterval(qint64 length) override;
        talcs::MetronomeAudioSourceDetectorMessage nextMessage() override;

        talcs::AudioFormatInputSource m_audio;
        talcs::MetronomeAudioSource m_metronome;
        talcs::AudioBuffer m_clickBuffer;
        QVector<QPointF> m_beats;
        double m_startSeconds{};
        double m_offsetSeconds{};
        double m_durationSeconds{};
        double m_playbackEndSeconds{};
        bool m_metronomeEnabled{true};
        qsizetype m_beatIndex{};
        qint64 m_intervalStart{};
        qint64 m_intervalEnd{};
    };

}

#endif // DIFFSCOPE_SYNTH_TEMPOEXTRACTIONAUDIOSOURCE_H
