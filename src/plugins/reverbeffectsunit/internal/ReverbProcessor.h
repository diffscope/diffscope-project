// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBPROCESSOR_H
#define DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBPROCESSOR_H

#include <atomic>
#include <cstdint>
#include <memory>

#include <TalcsCore/AudioSource.h>

namespace ReverbEffectsUnit::Internal {

    class ReverbProcessor : public talcs::AudioSource {
    public:
        ReverbProcessor();
        ~ReverbProcessor() override;

        void setParameters(double sizeMilliseconds, double decaySeconds,
                           double dampingPercent, double preDelayMilliseconds,
                           double mixPercent);
        void refresh();

        bool open(qint64 bufferSize, double sampleRate) override;
        void close() override;

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override;

    private:
        struct ParameterSnapshot {
            float sizeMilliseconds{};
            float decaySeconds{};
            float dampingPercent{};
            float preDelayMilliseconds{};
            float mixPercent{};
        };

        class ReverbEngine;

        bool tryReadParameterSnapshot(ParameterSnapshot &snapshot,
                                      std::uint64_t &revision) const;

        std::atomic<std::uint32_t> m_sizeMillisecondsBits{};
        std::atomic<std::uint32_t> m_decaySecondsBits{};
        std::atomic<std::uint32_t> m_dampingPercentBits{};
        std::atomic<std::uint32_t> m_preDelayMillisecondsBits{};
        std::atomic<std::uint32_t> m_mixPercentBits{};
        std::atomic<std::uint64_t> m_parameterRevision{};
        std::atomic<bool> m_refreshRequested{};
        std::unique_ptr<ReverbEngine> m_engine;
        std::uint64_t m_appliedRevision{};
    };

}

#endif // DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBPROCESSOR_H
