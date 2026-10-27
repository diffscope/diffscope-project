// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MidiPreviewStream.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

#include <TalcsCore/IAudioSampleContainer.h>

#include <audio/internal/WaveformSynthesizer.h>

namespace Audio::Internal {

    namespace {

        struct PreviewTestEvent {
            int position;
            int note;
            float velocity;
        };

        constexpr PreviewTestEvent previewTestScore[] = {
            {0, 47, 0.5f}, {0, 75, 1.0f},
            {2, 75, 0.0f}, {2, 66, 0.5f},
            {4, 66, 0.0f}, {4, 73, 0.7f},
            {6, 73, 0.0f}, {6, 71, 1.0f},
            {8, 71, 0.0f}, {8, 66, 0.5f},
            {10, 66, 0.0f}, {10, 73, 0.7f},
            {12, 47, 0.0f}, {12, 73, 0.0f},
            {12, 49, 0.5f}, {12, 75, 1.0f},
            {14, 75, 0.0f}, {14, 66, 0.5f},
            {15, 66, 0.0f}, {15, 76, 1.0f},
            {16, 76, 0.0f}, {16, 75, 0.7f},
            {18, 49, 0.0f}, {18, 75, 0.0f},
            {18, 42, 0.5f}, {18, 73, 1.0f},
            {24, 42, 0.0f}, {24, 73, 0.0f},
        };

        // Easter egg.
        constexpr PreviewTestEvent easterEggScore[] = {
            {0, 59, 0.3f}, {0, 66, 0.3f}, {0, 75, 1.0f},
            {4, 75, 0.0f}, {4, 73, 0.7f},
            {6, 73, 0.0f}, {6, 71, 1.0f},
            {10, 71, 0.0f}, {10, 73, 0.7f},
            {12, 73, 0.0f}, {12, 59, 0.0f}, {12, 66, 0.0f},
            {12, 52, 0.3f}, {12, 59, 0.3f}, {12, 75, 1.0f},
            {15, 75, 0.0f}, {15, 76, 1.0f},
            {16, 76, 0.0f}, {16, 75, 0.7f},
            {18, 75, 0.0f}, {18, 73, 1.0f},
            {24, 73, 0.0f}, {24, 52, 0.0f}, {24, 59, 0.0f},
        };

        constexpr quint64 testAlternativeFlag = 1;
        constexpr quint64 testPlayingFlag = 2;
        constexpr quint64 testRequestFlags = testAlternativeFlag | testPlayingFlag;
        constexpr double testPositionsPerSecond = 97.0 / 60.0 * 4.0;

    }

    bool MidiPreviewStream::open(qint64 bufferSize, double sampleRate) {
        m_voices.fill({});
        m_renderedRevision = m_configurationRevision.load();
        m_renderedTestRequest = m_testRequest.load();
        m_finishedTestRequest.store(m_renderedTestRequest);
        return AudioMidiStream::open(bufferSize, sampleRate);
    }

    void MidiPreviewStream::close() {
        m_voices.fill({});
        m_finishedTestRequest.store(m_testRequest.load());
        AudioMidiStream::close();
    }

    bool MidiPreviewStream::previewNeedsReset(bool enabled, int type) const {
        return m_enabled.load() != enabled || m_type.load() != type;
    }

    void MidiPreviewStream::configure(bool enabled, int type, double gain) {
        type = std::clamp(type, 0, waveformTypeCount - 1);
        const bool reset = previewNeedsReset(enabled, type);
        m_enabled.store(enabled);
        m_type.store(type);
        m_gain.store(gain);
        if (reset)
            m_configurationRevision.fetch_add(1);
    }

    void MidiPreviewStream::startTest(bool alternative) {
        const auto request = (m_testRequest.load() & ~testRequestFlags) + testRequestFlags + 1;
        m_testRequest.store(request | testPlayingFlag | (alternative ? testAlternativeFlag : 0));
    }

    void MidiPreviewStream::stopTest() {
        m_testRequest.store((m_testRequest.load() & ~testRequestFlags) + testRequestFlags + 1);
    }

    bool MidiPreviewStream::isTestPlaying() const {
        const auto request = m_testRequest.load();
        return (request & testPlayingFlag) && m_finishedTestRequest.load() != request;
    }

    void MidiPreviewStream::applyEvent(const talcs::IntegratedMidiMessage &event) {
        const auto &message = event.message;
        if (event.type != talcs::MidiInputEvent::Message) {
            for (auto &voice : m_voices) {
                if (voice.sourceId != m_testSourceId && (event.type == talcs::MidiInputEvent::AllSourcesStopped || voice.sourceId == event.sourceId))
                    voice.active = false;
            }
            return;
        }
        if (!message.isValid())
            return;
        const auto matches = [&](const Voice &voice) {
            return voice.active && voice.sourceId == event.sourceId && voice.channel == message.getChannel();
        };
        if (message.isAllNotesOff() || message.isAllSoundOff()) {
            for (auto &voice : m_voices) {
                if (matches(voice)) {
                    voice.held = false;
                    voice.noteOffTime = std::min(voice.time, voice.noteOffTime);
                    if (message.isAllSoundOff())
                        voice.active = false;
                }
            }
        } else if (message.isNoteOff()) {
            // Pair repeated notes in input order, independently for every source and channel.
            Voice *oldest = nullptr;
            for (auto &voice : m_voices) {
                if (matches(voice) && voice.held && voice.note == message.getNoteNumber() && (!oldest || voice.serial < oldest->serial))
                    oldest = &voice;
            }
            if (oldest) {
                oldest->held = false;
                oldest->noteOffTime = oldest->time;
            }
        } else if (message.isNoteOn()) {
            auto voice = std::find_if(m_voices.begin(), m_voices.end(), [](const Voice &voice) { return !voice.active; });
            if (voice == m_voices.end())
                voice = std::min_element(m_voices.begin(), m_voices.end(), [](const Voice &left, const Voice &right) { return left.serial < right.serial; });
            *voice = {true, true, event.sourceId, message.getChannel(), message.getNoteNumber(), m_type.load(),
                talcs::MidiMessage::getMidiNoteInHertz(message.getNoteNumber()), message.getFloatVelocity(),
                0.0, std::numeric_limits<double>::infinity(), 0.0, ++m_serial};
        }
    }

    qint64 MidiPreviewStream::processReading(const talcs::AudioSourceReadData &readData, const QList<talcs::IntegratedMidiMessage> &events) {
        for (int channel = 0; channel < readData.buffer->channelCount(); ++channel)
            readData.buffer->clear(channel, readData.startPos, readData.length);
        const auto revision = m_configurationRevision.load();
        if (revision != m_renderedRevision) {
            m_voices.fill({});
            m_renderedRevision = revision;
        }
        const auto testRequest = m_testRequest.load();
        if (testRequest != m_renderedTestRequest) {
            for (auto &voice : m_voices) {
                if (voice.sourceId == m_testSourceId)
                    voice.active = false;
            }
            m_renderedTestRequest = testRequest;
            m_testPosition = 0;
            m_testEventIndex = 0;
        }
        if (!m_enabled.load() || sampleRate() <= 0) {
            m_finishedTestRequest.store(testRequest);
            return readData.length;
        }
        bool testPlaying = (testRequest & testPlayingFlag) && m_finishedTestRequest.load() != testRequest;
        const auto testScore = (testRequest & testAlternativeFlag) ? std::span<const PreviewTestEvent>(easterEggScore) : std::span<const PreviewTestEvent>(previewTestScore);
        const auto gain = m_gain.load();
        auto event = events.cbegin();
        for (qint64 position = 0; position < readData.length; ++position) {
            while (event != events.cend() && event->position <= position)
                applyEvent(*event++);
            if (testPlaying) {
                while (m_testEventIndex < testScore.size() && m_testPosition >= std::llround(testScore[m_testEventIndex].position * sampleRate() / testPositionsPerSecond)) {
                    const auto &testEvent = testScore[m_testEventIndex++];
                    const auto message = testEvent.velocity > 0.0f ? talcs::MidiMessage::noteOn(1, testEvent.note, testEvent.velocity) : talcs::MidiMessage::noteOff(1, testEvent.note);
                    applyEvent({0, message, m_testSourceId});
                }
                ++m_testPosition;
            }
            double sample = 0.0;
            bool testVoiceActive = false;
            for (auto &voice : m_voices) {
                if (!voice.active)
                    continue;
                std::array<double, waveformTypeCount> weights{};
                weights[voice.type] = 1.0;
                sample += WaveformSynthesizer::render(voice.cycles, voice.frequency, voice.time, voice.noteOffTime,
                    voice.serial, weights, sampleRate()) * voice.velocity;
                voice.cycles += voice.frequency / sampleRate();
                voice.time += 1.0 / sampleRate();
                if (!voice.held && voice.time - voice.noteOffTime >= WaveformSynthesizer::maximumReleaseSeconds())
                    voice.active = false;
                testVoiceActive = testVoiceActive || (voice.active && voice.sourceId == m_testSourceId);
            }
            if (testPlaying && m_testEventIndex == testScore.size() && !testVoiceActive) {
                testPlaying = false;
                m_finishedTestRequest.store(testRequest);
            }
            if (readData.silentFlags == -1)
                continue;
            for (int channel = 0; channel < readData.buffer->channelCount(); ++channel) {
                if ((readData.silentFlags & (1 << channel)) == 0)
                    readData.buffer->setSample(channel, readData.startPos + position, static_cast<float>(sample * gain));
            }
        }
        return readData.length;
    }

}
