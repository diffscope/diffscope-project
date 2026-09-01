// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PitchShiftTask.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <rubberband/RubberBandStretcher.h>
#include <xxhash.h>

#include <QByteArray>
#include <QFile>
#include <QLoggingCategory>
#include <QSaveFile>

#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/AudioFormatIO.h>

namespace PitchShifter::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcPitchShiftTask, "diffscope.pitchshifter.task")

    namespace {

        constexpr std::size_t ProcessBlockSize = 16384;

        class CancelledException final : public std::exception {
        };

        class RubberBandLogger final : public RubberBand::RubberBandStretcher::Logger {
        public:
            void log(const char *message) override {
                qCDebug(lcPitchShiftTask) << "Rubber Band:" << message;
            }

            void log(const char *message, double value) override {
                qCDebug(lcPitchShiftTask) << "Rubber Band:" << message << value;
            }

            void log(const char *message, double value1, double value2) override {
                qCDebug(lcPitchShiftTask) << "Rubber Band:" << message << value1 << value2;
            }
        };

        [[noreturn]] void fail(const QString &message) {
            throw std::runtime_error(message.toUtf8().constData());
        }

        QString digest(const QString &filePath) {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                return {};
            }

            struct StateDeleter {
                void operator()(XXH3_state_t *state) const noexcept {
                    XXH3_freeState(state);
                }
            };
            std::unique_ptr<XXH3_state_t, StateDeleter> state(XXH3_createState());
            if (!state || XXH3_128bits_reset(state.get()) != XXH_OK) {
                return {};
            }

            std::array<char, 256 * 1024> buffer;
            while (true) {
                const auto size = file.read(buffer.data(), static_cast<qint64>(buffer.size()));
                if (size < 0) {
                    return {};
                }
                if (size == 0) {
                    break;
                }
                if (XXH3_128bits_update(state.get(), buffer.data(), static_cast<std::size_t>(size)) != XXH_OK) {
                    return {};
                }
            }

            const auto result = XXH3_128bits_digest(state.get());
            return QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(&result), sizeof(result)).toBase64(QByteArray::Base64UrlEncoding));
        }

    }

    PitchShiftTask::PitchShiftTask(std::unique_ptr<talcs::AbstractAudioFormatIO> inputIo, PitchShiftConfig config, QObject *parent)
        : QThread(parent), m_inputIo(std::move(inputIo)), m_config(std::move(config)) {
    }

    PitchShiftTask::~PitchShiftTask() {
        requestCancel();
        if (isRunning()) {
            wait();
        }
    }

    void PitchShiftTask::requestCancel() {
        m_cancelRequested.store(true, std::memory_order_relaxed);
    }

    void PitchShiftTask::run() {
        const auto checkCancelled = [this] {
            if (m_cancelRequested.load(std::memory_order_relaxed)) {
                throw CancelledException();
            }
        };

        try {
            qCInfo(lcPitchShiftTask) << "Starting pitch shift"
                                     << "pitch" << m_config.pitchSemitones
                                     << "formant mode" << m_config.formantMode
                                     << "formant shift" << m_config.formantShiftSemitones
                                     << "link channels" << m_config.linkChannels;
            Q_EMIT progressChanged(Preparing, 0.0);
            checkCancelled();

            if (!m_inputIo) {
                fail(tr("Unable to create an audio reader for the source file."));
            }
            if (m_inputIo->openMode() == talcs::AbstractAudioFormatIO::NotOpen
                && !m_inputIo->open(talcs::AbstractAudioFormatIO::Read)) {
                fail(tr("Unable to open the source audio file for reading."));
            }
            if (!(m_inputIo->openMode() & talcs::AbstractAudioFormatIO::Read)) {
                fail(tr("The source audio file is not open for reading."));
            }

            const auto sampleRate = m_inputIo->sampleRate();
            const auto channelCount = m_inputIo->channelCount();
            const auto inputLength = m_inputIo->length();
            if (sampleRate <= 0 || channelCount <= 0 || inputLength < 0) {
                fail(tr("The source audio file has invalid format information."));
            }

            auto options = RubberBand::RubberBandStretcher::OptionProcessOffline
                | RubberBand::RubberBandStretcher::OptionEngineFiner
                | RubberBand::RubberBandStretcher::OptionWindowStandard
                | RubberBand::RubberBandStretcher::OptionThreadingNever
                | (m_config.linkChannels
                       ? RubberBand::RubberBandStretcher::OptionChannelsTogether
                       : RubberBand::RubberBandStretcher::OptionChannelsApart);
            switch (m_config.formantMode) {
                case PitchShiftConfig::Preserve:
                    options |= RubberBand::RubberBandStretcher::OptionFormantPreserved;
                    break;
                case PitchShiftConfig::ShiftWithPitch:
                    options |= RubberBand::RubberBandStretcher::OptionFormantShifted;
                    break;
                case PitchShiftConfig::Custom:
                    options |= RubberBand::RubberBandStretcher::OptionFormantPreserved;
                    break;
            }

            const auto pitchScale = std::pow(2.0, m_config.pitchSemitones / 12.0);
            auto logger = std::make_shared<RubberBandLogger>();
            RubberBand::RubberBandStretcher stretcher(
                static_cast<std::size_t>(sampleRate),
                static_cast<std::size_t>(channelCount),
                logger,
                options,
                m_config.timeRatio,
                pitchScale
            );
            if (stretcher.getEngineVersion() != 3) {
                fail(tr("Rubber Band did not activate its highest-quality engine."));
            }
            stretcher.setDebugLevel(1);
            stretcher.setExpectedInputDuration(static_cast<std::size_t>(inputLength));
            stretcher.setMaxProcessSize(ProcessBlockSize);
            if (m_config.formantMode == PitchShiftConfig::Custom) {
                stretcher.setFormantScale(std::pow(2.0, (m_config.formantShiftSemitones - m_config.pitchSemitones) / 12.0));
            }

            std::vector<float> interleavedInput(ProcessBlockSize * static_cast<std::size_t>(channelCount));
            std::vector<std::vector<float>> channelInput(static_cast<std::size_t>(channelCount), std::vector<float>(ProcessBlockSize));
            std::vector<const float *> inputPointers(static_cast<std::size_t>(channelCount));
            for (int channel = 0; channel < channelCount; ++channel) {
                inputPointers[static_cast<std::size_t>(channel)] = channelInput[static_cast<std::size_t>(channel)].data();
            }

            const auto readInput = [&](qint64 requestedFrames) {
                const auto readFrames = m_inputIo->read(interleavedInput.data(), requestedFrames);
                if (readFrames <= 0) {
                    fail(tr("Unexpected end of the source audio file."));
                }
                for (qint64 frame = 0; frame < readFrames; ++frame) {
                    for (int channel = 0; channel < channelCount; ++channel) {
                        channelInput[static_cast<std::size_t>(channel)][static_cast<std::size_t>(frame)] =
                            interleavedInput[static_cast<std::size_t>(frame) * static_cast<std::size_t>(channelCount) + static_cast<std::size_t>(channel)];
                    }
                }
                return readFrames;
            };

            if (m_inputIo->seek(0) != 0) {
                fail(tr("Unable to seek in the source audio file."));
            }
            qCDebug(lcPitchShiftTask) << "Starting Rubber Band analysis pass";
            Q_EMIT progressChanged(Analyzing, 0.0);
            qint64 analyzedFrames = 0;
            if (inputLength == 0) {
                stretcher.study(nullptr, 0, true);
            }
            while (analyzedFrames < inputLength) {
                checkCancelled();
                const auto requestedFrames = std::min<qint64>(static_cast<qint64>(ProcessBlockSize), inputLength - analyzedFrames);
                const auto readFrames = readInput(requestedFrames);
                analyzedFrames += readFrames;
                stretcher.study(inputPointers.data(), static_cast<std::size_t>(readFrames), analyzedFrames == inputLength);
            }

            checkCancelled();
            if (m_inputIo->seek(0) != 0) {
                fail(tr("Unable to rewind the source audio file."));
            }

            QSaveFile outputFile(m_config.outputFilePath);
            outputFile.setDirectWriteFallback(false);
            if (!outputFile.open(QIODevice::WriteOnly)) {
                fail(tr("Unable to open the destination file for writing: %1").arg(outputFile.errorString()));
            }
            talcs::AudioFormatIO outputIo(&outputFile);
            const auto outputFormat = static_cast<int>(talcs::AudioFormatIO::WAV) | static_cast<int>(talcs::AudioFormatIO::FLOAT);
            if (!outputIo.AbstractAudioFormatIO::open(talcs::AbstractAudioFormatIO::Write, outputFormat, channelCount, sampleRate)) {
                fail(tr("Unable to create the destination WAV file: %1").arg(outputIo.errorString()));
            }

            std::vector<std::vector<float>> channelOutput(static_cast<std::size_t>(channelCount), std::vector<float>(ProcessBlockSize));
            std::vector<float *> outputPointers(static_cast<std::size_t>(channelCount));
            for (int channel = 0; channel < channelCount; ++channel) {
                outputPointers[static_cast<std::size_t>(channel)] = channelOutput[static_cast<std::size_t>(channel)].data();
            }
            std::vector<float> interleavedOutput(ProcessBlockSize * static_cast<std::size_t>(channelCount));

            qint64 outputFrames = 0;
            const auto retrieveOutput = [&] {
                while (true) {
                    const auto availableFrames = stretcher.available();
                    if (availableFrames <= 0) {
                        break;
                    }
                    checkCancelled();
                    const auto framesToRetrieve = std::min<std::size_t>(static_cast<std::size_t>(availableFrames), ProcessBlockSize);
                    const auto retrievedFrames = stretcher.retrieve(outputPointers.data(), framesToRetrieve);
                    if (retrievedFrames == 0) {
                        fail(tr("Rubber Band failed to return processed audio."));
                    }
                    for (std::size_t frame = 0; frame < retrievedFrames; ++frame) {
                        for (int channel = 0; channel < channelCount; ++channel) {
                            interleavedOutput[frame * static_cast<std::size_t>(channelCount) + static_cast<std::size_t>(channel)] =
                                channelOutput[static_cast<std::size_t>(channel)][frame];
                        }
                    }
                    if (outputIo.write(interleavedOutput.data(), static_cast<qint64>(retrievedFrames)) != static_cast<qint64>(retrievedFrames)) {
                        fail(tr("Unable to write all processed audio to the destination file."));
                    }
                    outputFrames += static_cast<qint64>(retrievedFrames);
                }
            };

            qCDebug(lcPitchShiftTask) << "Starting Rubber Band processing pass";
            Q_EMIT progressChanged(Processing, 0.0);
            qint64 processedFrames = 0;
            if (inputLength == 0) {
                stretcher.process(nullptr, 0, true);
                retrieveOutput();
            }
            while (processedFrames < inputLength) {
                checkCancelled();
                const auto requestedFrames = std::min<qint64>(static_cast<qint64>(ProcessBlockSize), inputLength - processedFrames);
                const auto readFrames = readInput(requestedFrames);
                processedFrames += readFrames;
                stretcher.process(inputPointers.data(), static_cast<std::size_t>(readFrames), processedFrames == inputLength);
                retrieveOutput();
                Q_EMIT progressChanged(Processing, static_cast<double>(processedFrames) / static_cast<double>(inputLength));
            }
            retrieveOutput();

            checkCancelled();
            qCDebug(lcPitchShiftTask) << "Finalizing destination audio";
            Q_EMIT progressChanged(Finalizing, 1.0);
            outputIo.close();
            m_inputIo->close();
            m_inputIo.reset();
            if (!outputFile.commit()) {
                fail(tr("Unable to commit the destination file: %1").arg(outputFile.errorString()));
            }

            const auto outputDigest = digest(m_config.outputFilePath);
            if (outputDigest.isEmpty()) {
                fail(tr("Unable to calculate the destination file digest."));
            }
            Q_EMIT progressChanged(Finalizing, 1.0);
            qCInfo(lcPitchShiftTask) << "Pitch shift completed";
            Q_EMIT succeeded(outputDigest);
        } catch (const CancelledException &) {
            if (m_inputIo) {
                m_inputIo->close();
                m_inputIo.reset();
            }
            qCWarning(lcPitchShiftTask) << "Pitch shift cancelled";
            Q_EMIT cancelled();
        } catch (const std::exception &exception) {
            if (m_inputIo) {
                m_inputIo->close();
                m_inputIo.reset();
            }
            const auto message = QString::fromUtf8(exception.what());
            qCCritical(lcPitchShiftTask) << "Pitch shift failed:" << message;
            Q_EMIT failed(message);
        } catch (...) {
            if (m_inputIo) {
                m_inputIo->close();
                m_inputIo.reset();
            }
            const auto message = tr("An unknown error occurred while stretching and shifting pitch.");
            qCCritical(lcPitchShiftTask) << "Pitch shift failed with an unknown exception";
            Q_EMIT failed(message);
        }
    }

}

#include "moc_PitchShiftTask.cpp"
