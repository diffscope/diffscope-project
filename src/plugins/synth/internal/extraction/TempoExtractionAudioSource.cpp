// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TempoExtractionAudioSource.h"

#include <algorithm>

namespace Synth::Internal {

    TempoExtractionAudioSource::TempoExtractionAudioSource(talcs::AbstractAudioFormatIO *io, double offsetSeconds, double durationSeconds)
        : m_audio(io, true), m_offsetSeconds(offsetSeconds), m_durationSeconds(durationSeconds), m_playbackEndSeconds(durationSeconds) {
        m_metronome.setMajorBeatSource(talcs::MetronomeAudioSource::builtInMajorBeatSource(), true);
        m_metronome.setMinorBeatSource(talcs::MetronomeAudioSource::builtInMinorBeatSource(), true);
        m_metronome.setDetector(this);
    }

    bool TempoExtractionAudioSource::open(qint64 bufferSize, double sampleRate) {
        close();
        if (!m_audio.open(bufferSize, sampleRate) || !m_metronome.open(bufferSize, sampleRate)) {
            close();
            return false;
        }
        m_clickBuffer.resize(2, bufferSize);
        talcs::PositionableAudioSource::open(bufferSize, sampleRate);
        setNextReadPosition(nextReadPosition());
        return true;
    }

    void TempoExtractionAudioSource::close() {
        m_audio.close();
        m_metronome.close();
        talcs::PositionableAudioSource::close();
    }

    qint64 TempoExtractionAudioSource::length() const {
        return qRound64(std::min(m_durationSeconds, m_playbackEndSeconds) * sampleRate());
    }

    void TempoExtractionAudioSource::setNextReadPosition(qint64 position) {
        const bool moved = position != nextReadPosition();
        talcs::PositionableAudioSource::setNextReadPosition(position);
        if (isOpen()) {
            m_audio.setNextReadPosition(qRound64(m_offsetSeconds * sampleRate()) + position);
            if (moved) {
                m_metronome.majorBeatSource()->setNextReadPosition(m_metronome.majorBeatSource()->length());
                m_metronome.minorBeatSource()->setNextReadPosition(m_metronome.minorBeatSource()->length());
            }
        }
    }

    void TempoExtractionAudioSource::setBeats(const QVector<QPointF> &beats, double startSeconds) {
        m_beats = beats;
        m_startSeconds = startSeconds;
    }

    void TempoExtractionAudioSource::setMetronomeEnabled(bool enabled) {
        m_metronomeEnabled = enabled;
    }

    void TempoExtractionAudioSource::setPlaybackEnd(double seconds) {
        m_playbackEndSeconds = seconds;
    }

    qint64 TempoExtractionAudioSource::processReading(const talcs::AudioSourceReadData &readData) {
        const auto position = nextReadPosition();
        const auto count = std::clamp(length() - position, qint64(0), readData.length);
        m_audio.read({readData.buffer, readData.startPos, count, readData.silentFlags});
        if (m_metronomeEnabled && count > 0) {
            m_intervalStart = position;
            m_metronome.read({&m_clickBuffer, 0, count});
            for (int channel = 0; channel < readData.buffer->channelCount(); ++channel) {
                if (!(readData.silentFlags & (1 << channel)))
                    readData.buffer->addSampleRange(channel, readData.startPos, count, m_clickBuffer, channel % 2, 0, 0.5f);
            }
        }
        for (int channel = 0; channel < readData.buffer->channelCount(); ++channel)
            readData.buffer->clear(channel, readData.startPos + count, readData.length - count);
        talcs::PositionableAudioSource::setNextReadPosition(position + count);
        return count;
    }

    void TempoExtractionAudioSource::detectInterval(qint64 length) {
        m_intervalEnd = m_intervalStart + length;
        const auto seconds = m_startSeconds + (m_intervalStart - 0.5) / sampleRate();
        m_beatIndex = std::lower_bound(m_beats.cbegin(), m_beats.cend(), seconds, [](const QPointF &beat, double time) { return beat.x() < time; }) - m_beats.cbegin();
    }

    talcs::MetronomeAudioSourceDetectorMessage TempoExtractionAudioSource::nextMessage() {
        while (m_beatIndex < m_beats.size()) {
            const auto &beat = m_beats.at(m_beatIndex);
            const auto position = qRound64((beat.x() - m_startSeconds) * sampleRate());
            if (position >= m_intervalEnd)
                break;
            ++m_beatIndex;
            if (position >= m_intervalStart)
                return {position - m_intervalStart, beat.y() != 0};
        }
        return {-1, false};
    }

}
