// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ExtractionAudioCodec.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <QBuffer>
#include <QCoreApplication>
#include <QFile>
#include <QMimeDatabase>
#include <QTemporaryDir>

#include <TalcsCore/AudioBuffer.h>
#include <TalcsFormat/AudioFormatIO.h>
#include <TalcsFormat/AudioFormatInputSource.h>
#include <TalcsFormat/FormatManager.h>

#include <audio/GlobalAudioContext.h>

namespace Synth::Internal::ExtractionAudioCodec {

    namespace {

        QString message(const char *text) {
            return QCoreApplication::translate("Synth::Internal::ExtractionAudioCodec", text);
        }

        Result transcode(talcs::AbstractAudioFormatIO &input, int format, const QString &mime,
                         double sampleRate, double offset, double duration,
                         const std::shared_ptr<std::atomic_bool> &cancelled) {
            if (!(input.openMode() & talcs::AbstractAudioFormatIO::Read) && !input.open(talcs::AbstractAudioFormatIO::Read))
                return {{}, {}, message(QT_TRANSLATE_NOOP("Synth::Internal::ExtractionAudioCodec", "Could not open the source audio."))};
            if (!std::isfinite(sampleRate) || sampleRate <= 0 || input.channelCount() <= 0
                || input.channelCount() > 256 || !std::isfinite(offset) || offset < 0
                || !std::isfinite(duration) || duration <= 0
                || (offset + duration) * sampleRate >= static_cast<double>(std::numeric_limits<qint64>::max())) {
                return {{}, {}, message(QT_TRANSLATE_NOOP("Synth::Internal::ExtractionAudioCodec", "The audio has invalid format information."))};
            }
            QBuffer output;
            output.open(QIODevice::ReadWrite);
            talcs::AudioFormatIO writer(&output);
            writer.setFormat(format);
            writer.setChannelCount(input.channelCount());
            writer.setSampleRate(sampleRate);
            if (!writer.open(talcs::AbstractAudioFormatIO::Write))
                return {{}, {}, writer.errorString()};

            constexpr qint64 blockSize = 4096;
            talcs::AudioFormatInputSource source(&input);
            source.setStereoize(false);
            if (!source.open(blockSize, sampleRate))
                return {{}, {}, message(QT_TRANSLATE_NOOP("Synth::Internal::ExtractionAudioCodec", "Could not prepare the audio for extraction."))};
            source.setNextReadPosition(std::llround(offset * sampleRate));
            const auto length = std::llround(duration * sampleRate);
            talcs::AudioBuffer buffer(input.channelCount(), blockSize);
            std::vector<float> interleaved(static_cast<size_t>(blockSize) * input.channelCount());
            for (qint64 position = 0; position < length;) {
                if (cancelled->load())
                    return {};
                const auto count = std::min(blockSize, length - position);
                source.read({&buffer, 0, count});
                for (qint64 frame = 0; frame < count; ++frame) {
                    for (int channel = 0; channel < input.channelCount(); ++channel)
                        interleaved[frame * input.channelCount() + channel] = buffer.sample(channel, frame);
                }
                if (writer.write(interleaved.data(), count) != count)
                    return {{}, {}, message(QT_TRANSLATE_NOOP("Synth::Internal::ExtractionAudioCodec", "Could not convert the audio format."))};
                position += count;
            }
            writer.close();
            return {output.data(), mime, {}};
        }

    }

    Result encode(const Input &input, const Api::V1::ExtractorInfo &extractor,
                  const std::shared_ptr<std::atomic_bool> &cancelled) {
        std::unique_ptr<talcs::AbstractAudioFormatIO> reader(
            Audio::GlobalAudioContext::formatManager()->getFormatLoad(input.filePath, input.userData, input.formatEntryClassName));
        if (!reader || (!(reader->openMode() & talcs::AbstractAudioFormatIO::Read) && !reader->open(talcs::AbstractAudioFormatIO::Read)))
            return {{}, {}, message(QT_TRANSLATE_NOOP("Synth::Internal::ExtractionAudioCodec", "Could not open the source audio."))};
        if (!std::isfinite(reader->sampleRate()) || reader->sampleRate() <= 0 || reader->length() <= 0)
            return {{}, {}, message(QT_TRANSLATE_NOOP("Synth::Internal::ExtractionAudioCodec", "The audio has invalid format information."))};

        int rate = extractor.preferredAudioSampleRate;
        if (!extractor.acceptableAudioSampleRates.isEmpty() && !extractor.acceptableAudioSampleRates.contains(rate))
            rate = extractor.acceptableAudioSampleRates.first();
        if (!extractor.acceptableSchemes.isEmpty() && !extractor.acceptableSchemes.contains(QStringLiteral("data"), Qt::CaseInsensitive))
            return {{}, {}, message(QT_TRANSLATE_NOOP("Synth::Internal::ExtractionAudioCodec", "The extractor does not accept audio data URLs."))};

        auto formats = talcs::AudioFormatIO::availableFormats();
        std::stable_sort(formats.begin(), formats.end(), [](const auto &left, const auto &right) {
            return left.majorFormat == talcs::AudioFormatIO::WAV && right.majorFormat != talcs::AudioFormatIO::WAV;
        });
        QMimeDatabase database;
        for (const auto &format : formats) {
            if (cancelled->load())
                return {};
            if (format.majorFormat == talcs::AudioFormatIO::RAW)
                continue;
            // Try every subtype supported by the encoder for this container and channel layout.
            auto subtypes = format.subtypes;
            std::stable_sort(subtypes.begin(), subtypes.end(), [](const auto &left, const auto &right) {
                return left.subtype == talcs::AudioFormatIO::FLOAT && right.subtype != talcs::AudioFormatIO::FLOAT;
            });
            for (const auto &subtype : subtypes) {
                auto extensions = subtype.extensions;
                extensions.prepend(format.extension);
                QString selectedMime;
                for (const auto &extension : extensions) {
                    const auto mime = database.mimeTypeForFile(QStringLiteral("audio.") + extension, QMimeDatabase::MatchExtension);
                    if (extractor.acceptableFormats.isEmpty())
                        selectedMime = mime.name();
                    for (const auto &accepted : extractor.acceptableFormats) {
                        if (accepted == QStringLiteral("*") || accepted == QStringLiteral("*/*")
                            || (accepted == QStringLiteral("audio/*") && mime.name().startsWith(QStringLiteral("audio/"))))
                            selectedMime = mime.name();
                        else if (database.mimeTypeForName(accepted) == mime || accepted.compare(extension, Qt::CaseInsensitive) == 0)
                            selectedMime = accepted.contains(u'/') ? accepted : mime.name();
                        if (!selectedMime.isEmpty())
                            break;
                    }
                    if (!selectedMime.isEmpty())
                        break;
                }
                if (selectedMime.isEmpty())
                    continue;
                auto result = transcode(*reader, format.majorFormat | subtype.subtype, selectedMime, rate,
                                        input.offsetSeconds, input.durationSeconds, cancelled);
                if (cancelled->load() || !result.bytes.isEmpty())
                    return result;
            }
        }
        return {{}, {}, message(QT_TRANSLATE_NOOP("Synth::Internal::ExtractionAudioCodec", "Could not encode an audio format accepted by the extractor."))};
    }

    Result decode(const QByteArray &bytes, const std::shared_ptr<std::atomic_bool> &cancelled) {
        QBuffer input;
        input.setData(bytes);
        input.open(QIODevice::ReadOnly);
        talcs::AudioFormatIO reader(&input);
        if (reader.open(talcs::AbstractAudioFormatIO::Read) && reader.sampleRate() > 0 && reader.length() > 0) {
            return transcode(reader, talcs::AudioFormatIO::WAV | talcs::AudioFormatIO::FLOAT,
                             QStringLiteral("audio/wav"), reader.sampleRate(), 0,
                             reader.length() / reader.sampleRate(), cancelled);
        }
        if (cancelled->load())
            return {};
        QTemporaryDir directory;
        const auto suffix = QMimeDatabase().mimeTypeForData(bytes).preferredSuffix();
        const auto path = directory.filePath(QStringLiteral("audio.") + suffix);
        QFile file(path);
        if (directory.isValid() && file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.flush()) {
            file.close();
            std::unique_ptr<talcs::AbstractAudioFormatIO> decoder(Audio::GlobalAudioContext::formatManager()->getFormatLoad(path));
            if (decoder && decoder->sampleRate() > 0 && decoder->length() > 0) {
                return transcode(*decoder, talcs::AudioFormatIO::WAV | talcs::AudioFormatIO::FLOAT,
                                 QStringLiteral("audio/wav"), decoder->sampleRate(), 0,
                                 decoder->length() / decoder->sampleRate(), cancelled);
            }
        }
        return {{}, {}, message(QT_TRANSLATE_NOOP("Synth::Internal::ExtractionAudioCodec", "Could not decode the separated audio."))};
    }

}
