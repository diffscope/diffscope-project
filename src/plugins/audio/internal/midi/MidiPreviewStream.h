// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_MIDIPREVIEWSTREAM_H
#define DIFFSCOPE_AUDIO_MIDIPREVIEWSTREAM_H

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <TalcsMidi/AudioMidiStream.h>
#include <TalcsMidi/MidiInputEvent.h>

namespace Audio::Internal {

    class MidiPreviewStream : public talcs::AudioMidiStream {
    public:
        bool open(qint64 bufferSize, double sampleRate) override;
        void close() override;
        void configure(bool enabled, int type, double gain);
        bool previewNeedsReset(bool enabled, int type) const;
        void startTest(bool alternative);
        void stopTest();
        bool isTestPlaying() const;

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData, const QList<talcs::IntegratedMidiMessage> &events) override;

    private:
        struct Voice {
            bool active{};
            bool held{};
            quint64 sourceId{};
            int channel{};
            int note{};
            int type{};
            double frequency{};
            double velocity{};
            double time{};
            double noteOffTime{};
            double cycles{};
            std::uint64_t serial{};
        };

        void applyEvent(const talcs::IntegratedMidiMessage &event);

        std::array<Voice, 128> m_voices{};
        std::atomic<bool> m_enabled{true};
        std::atomic<int> m_type{};
        std::atomic<double> m_gain{1.0};
        std::atomic<quint64> m_configurationRevision{};
        quint64 m_renderedRevision{};
        std::uint64_t m_serial{};
        const quint64 m_testSourceId{talcs::MidiInputEvent::createSourceId()};
        std::atomic<quint64> m_testRequest{};
        std::atomic<quint64> m_finishedTestRequest{};
        quint64 m_renderedTestRequest{};
        qint64 m_testPosition{};
        std::size_t m_testEventIndex{};
    };

}

#endif // DIFFSCOPE_AUDIO_MIDIPREVIEWSTREAM_H
