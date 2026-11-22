// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EmbeddedAudioCodec.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <vector>

#include <sndfile.h>
#include <wavpack/wavpack.h>
#include <xxhash.h>

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QTemporaryFile>

namespace Audio::Internal {

    using Compression = EmbeddedAudio::Compression;
    using SampleType = EmbeddedAudio::SampleType;
    using WavPack = std::unique_ptr<WavpackContext, decltype(&WavpackCloseFile)>;
    using SoundFile = std::unique_ptr<SNDFILE, decltype(&sf_close)>;

    static QByteArray waveHeader(const EmbeddedAudio::StreamFormat &format, int bits, bool floating, quint64 dataSize) {
        const bool extensible = bits >= 24 || format.channelCount > 2;
        const quint32 fmtSize = extensible ? 40 : 16;
        const quint64 riffSize = 4 + 8 + fmtSize + (floating ? 12 : 0) + 8 + dataSize + (dataSize & 1);
        const bool rf64 = riffSize > std::numeric_limits<quint32>::max();
        const auto blockAlign = static_cast<quint16>(format.channelCount * (bits / 8));
        const quint32 channelMask = format.channelCount == 1 ? 4 : format.channelCount == 2 ? 3 : 0;
        QByteArray header;
        QDataStream out(&header, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out.writeRawData(rf64 ? "RF64" : "RIFF", 4);
        out << (rf64 ? 0xffffffffu : static_cast<quint32>(riffSize));
        out.writeRawData("WAVE", 4);
        if (rf64) {
            out.writeRawData("ds64", 4);
            out << quint32(28) << quint64(riffSize + 36) << dataSize << quint64(format.frameCount) << quint32(0);
        }
        out.writeRawData("fmt ", 4);
        out << fmtSize << quint16(extensible ? 0xfffe : 1) << quint16(format.channelCount) << quint32(format.sampleRate)
            << quint32(quint64(format.sampleRate) * blockAlign) << blockAlign << quint16(bits);
        if (extensible) {
            out << quint16(22) << quint16(bits) << channelMask << quint32(floating ? 3 : 1) << quint16(0) << quint16(0x0010);
            out.writeRawData("\x80\x00\x00\xaa\x00\x38\x9b\x71", 8);
        }
        if (floating) {
            out.writeRawData("fact", 4);
            out << quint32(4) << quint32(std::min<quint64>(format.frameCount, 0xffffffffu));
        }
        out.writeRawData("data", 4);
        out << (rf64 ? 0xffffffffu : static_cast<quint32>(dataSize));
        return header;
    }

    static void writePcmSample(char *&out, int32_t sample, int bytes) {
        const auto storage = static_cast<quint32>(bytes == 1 ? sample + 128 : sample);
        for (int byte = 0; byte < bytes; ++byte) {
            *out++ = static_cast<char>((storage >> (byte * 8)) & 0xffu);
        }
    }

    static SF_VIRTUAL_IO soundFileIO = {
        .get_filelen = [](void *file) -> sf_count_t { return static_cast<QFileDevice *>(file)->size(); },
        .seek = [](sf_count_t offset, int origin, void *file) -> sf_count_t {
            auto *device = static_cast<QFileDevice *>(file);
            const auto base = origin == SEEK_CUR ? device->pos() : origin == SEEK_END ? device->size() : 0;
            return offset >= -base && offset <= std::numeric_limits<qint64>::max() - base && device->seek(base + offset) ? device->pos() : -1;
        },
        .read = [](void *data, sf_count_t size, void *file) -> sf_count_t {
            return std::max<qint64>(0, static_cast<QFileDevice *>(file)->read(static_cast<char *>(data), size));
        },
        .write = [](const void *data, sf_count_t size, void *file) -> sf_count_t {
            return std::max<qint64>(0, static_cast<QFileDevice *>(file)->write(static_cast<const char *>(data), size));
        },
        .tell = [](void *file) -> sf_count_t { return static_cast<QFileDevice *>(file)->pos(); },
    };

    EmbeddedAudioFile::EmbeddedAudioFile(QTemporaryFile &file, Compression compression) : path(file.fileName()), compression(compression) {
        file.setAutoRemove(false);
    }

    EmbeddedAudioFile::~EmbeddedAudioFile() {
        QFile::remove(path);
    }

    EmbeddedAudioEncoded encodeEmbeddedAudio(const EmbeddedAudio::StreamFormat &format, const EmbeddedAudio::FrameReader &reader,
                                             Compression compression, double pcmScale, const std::function<bool()> &cancelled) {
        const bool floating = format.sampleType == SampleType::Float32 || format.sampleType == SampleType::Other;
        const int bits = format.sampleType == SampleType::Int8 ? 8 : format.sampleType == SampleType::Int16 ? 16 : format.sampleType == SampleType::Int24 ? 24 : 32;
        const int bytes = bits / 8;
        const auto channels = format.channelCount;
        if (!reader || format.sampleRate <= 0 || channels <= 0 || channels > 4096
            || quint64(channels) * bytes * format.sampleRate > std::numeric_limits<quint32>::max()) {
            return {};
        }

        const auto chunkFrames = 65536 / channels;
        std::vector<float> frames(chunkFrames * channels);
        if (format.frameCount == -1) {
            QTemporaryFile spool;
            if (!spool.open()) {
                return {};
            }
            auto sizedFormat = format;
            sizedFormat.frameCount = 0;
            while (!cancelled()) {
                const auto count = reader(frames.data(), chunkFrames);
                if (count == 0) {
                    break;
                }
                if (count < 0 || count > chunkFrames || sizedFormat.frameCount > MAX_WAVPACK_SAMPLES - count) {
                    return {};
                }
                const auto size = qint64(count * channels * sizeof(float));
                if (spool.write(reinterpret_cast<const char *>(frames.data()), size) != size) {
                    return {};
                }
                sizedFormat.frameCount += count;
            }
            if (cancelled() || !spool.flush() || !spool.seek(0)) {
                return {};
            }
            return encodeEmbeddedAudio(sizedFormat, [&](float *data, qint64 count) {
                const auto size = qint64(count * channels * sizeof(float));
                const auto read = spool.read(reinterpret_cast<char *>(data), size);
                return read == size ? count : read == 0 ? 0 : -1;
            }, compression, pcmScale, cancelled);
        }

        if (format.frameCount <= 0 || format.frameCount > MAX_WAVPACK_SAMPLES || cancelled()) {
            return {};
        }
        const auto dataSize = quint64(format.frameCount) * channels * bytes;
        auto header = waveHeader(format, bits, floating, dataSize);
        std::unique_ptr<XXH3_state_t, decltype(&XXH3_freeState)> hash(XXH3_createState(), XXH3_freeState);
        if (!hash || XXH3_128bits_reset(hash.get()) != XXH_OK
            || XXH3_128bits_update(hash.get(), header.constData(), header.size()) != XXH_OK) {
            return {};
        }

        const bool lossless = compression == Compression::Lossless;
        QTemporaryFile output(QDir::temp().filePath(lossless ? QStringLiteral("diffscope-XXXXXX.wv") : QStringLiteral("diffscope-XXXXXX.ogg")));
        if (!output.open()) {
            return {};
        }
        WavPack wavPack(nullptr, WavpackCloseFile);
        SoundFile vorbis(nullptr, sf_close);
        if (lossless) {
            wavPack.reset(WavpackOpenFileOutput([](void *file, void *data, int32_t size) {
                return int(static_cast<QFileDevice *>(file)->write(static_cast<const char *>(data), size) == size);
            }, &output, nullptr));
            WavpackConfig config{};
            config.bits_per_sample = bits;
            config.bytes_per_sample = bytes;
            config.float_norm_exp = floating ? 127 : 0;
            config.num_channels = channels;
            config.sample_rate = format.sampleRate;
            config.channel_mask = channels == 1 ? 4 : channels == 2 ? 3 : 0;
            if (!wavPack || !WavpackSetConfiguration64(wavPack.get(), &config, format.frameCount, nullptr)
                || !WavpackAddWrapper(wavPack.get(), header.data(), header.size()) || !WavpackPackInit(wavPack.get())) {
                return {};
            }
        } else {
            SF_INFO info{};
            info.samplerate = format.sampleRate;
            info.channels = channels;
            info.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
            if (!sf_format_check(&info)) {
                return {};
            }
            vorbis.reset(sf_open_virtual(&soundFileIO, SFM_WRITE, &info, &output));
            double quality = 1.0;
            if (!vorbis || sf_command(vorbis.get(), SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof(quality)) != SF_TRUE) {
                return {};
            }
        }

        std::vector<int32_t> wavPackSamples(frames.size());
        QByteArray pcm(frames.size() * bytes, Qt::Uninitialized);
        const auto scale = pcmScale == 0.0 ? std::ldexp(1.0, bits - 1) : pcmScale;
        const auto limit = qint64(1) << (bits - 1);
        qint64 position = 0;
        while (position < format.frameCount) {
            if (cancelled()) {
                return {};
            }
            const auto remaining = std::min<qint64>(chunkFrames, format.frameCount - position);
            const auto count = reader(frames.data(), remaining);
            if (count <= 0 || count > remaining) {
                return {};
            }
            char *out = pcm.data();
            for (qsizetype i = 0; i < count * channels; ++i) {
                int32_t sample;
                if (floating) {
                    sample = std::bit_cast<int32_t>(frames[i]);
                } else {
                    const auto value = std::round(double(frames[i]) * scale);
                    if (!std::isfinite(value) || value < -limit || value >= limit
                        || static_cast<float>(value / scale) != frames[i]) {
                        return {};
                    }
                    sample = static_cast<int32_t>(value);
                }
                wavPackSamples[i] = sample;
                writePcmSample(out, sample, bytes);
            }
            if (XXH3_128bits_update(hash.get(), pcm.constData(), out - pcm.data()) != XXH_OK
                || (wavPack && !WavpackPackSamples(wavPack.get(), wavPackSamples.data(), count))
                || (vorbis && sf_writef_float(vorbis.get(), frames.data(), count) != count)) {
                return {};
            }
            position += count;
        }
        if (cancelled() || reader(frames.data(), 1) != 0) {
            return {};
        }
        if (wavPack && !WavpackFlushSamples(wavPack.get())) {
            return {};
        }
        if (dataSize & 1) {
            char zero = 0;
            if (XXH3_128bits_update(hash.get(), &zero, 1) != XXH_OK
                || (wavPack && (!WavpackAddWrapper(wavPack.get(), &zero, 1) || !WavpackFlushSamples(wavPack.get())))) {
                return {};
            }
        }
        if (vorbis && sf_close(vorbis.release()) != 0) {
            return {};
        }
        wavPack.reset();
        if (!output.flush() || output.error() != QFileDevice::NoError) {
            return {};
        }
        XXH128_canonical_t canonical;
        XXH128_canonicalFromHash(&canonical, XXH3_128bits_digest(hash.get()));
        const auto hex = QByteArray(reinterpret_cast<const char *>(canonical.digest), sizeof(canonical.digest)).toHex();
        output.close();
        return {QString::fromLatin1(hex), QSharedPointer<EmbeddedAudioFile>::create(output, compression)};
    }

    bool extractWavPack(const QString &path, QIODevice &output) {
#ifdef Q_OS_WIN
        const auto fileName = QDir::toNativeSeparators(path).toUtf8();
#else
        const auto fileName = QFile::encodeName(path);
#endif
        char error[80]{};
        WavPack input(WavpackOpenFileInput(fileName.constData(), error, OPEN_FILE_UTF8, 0), WavpackCloseFile);
        if (!input || !(WavpackGetMode(input.get()) & MODE_LOSSLESS)) {
            return false;
        }

        const auto channels = WavpackGetNumChannels(input.get());
        const auto sampleRate = WavpackGetSampleRate(input.get());
        const auto frameCount = WavpackGetNumSamples64(input.get());
        const auto bytes = WavpackGetBytesPerSample(input.get());
        const auto bits = WavpackGetBitsPerSample(input.get());
        const bool floating = WavpackGetMode(input.get()) & MODE_FLOAT;
        if (channels <= 0 || channels > 4096 || sampleRate == 0 || sampleRate > std::numeric_limits<int>::max()
            || frameCount <= 0 || frameCount > MAX_WAVPACK_SAMPLES || bytes < 1 || bytes > 4 || bits != bytes * 8
            || (floating && bytes != 4) || quint64(channels) * bytes * sampleRate > std::numeric_limits<quint32>::max()) {
            return false;
        }

        const EmbeddedAudio::StreamFormat format{.sampleRate = static_cast<int>(sampleRate), .channelCount = channels, .frameCount = frameCount};
        const auto dataSize = quint64(frameCount) * channels * bytes;
        const auto header = waveHeader(format, bits, floating, dataSize);
        if (output.write(header) != header.size()) {
            return false;
        }

        const auto chunkFrames = 65536 / channels;
        std::vector<int32_t> samples(chunkFrames * channels);
        QByteArray pcm(samples.size() * bytes, Qt::Uninitialized);
        for (qint64 remaining = frameCount; remaining > 0;) {
            const auto count = static_cast<quint32>(std::min<qint64>(remaining, chunkFrames));
            if (WavpackUnpackSamples(input.get(), samples.data(), count) != count) {
                return false;
            }
            auto *out = pcm.data();
            for (qsizetype i = 0; i < qsizetype(count) * channels; ++i) {
                writePcmSample(out, samples[i], bytes);
            }
            const auto size = out - pcm.constData();
            if (output.write(pcm.constData(), size) != size) {
                return false;
            }
            remaining -= count;
        }
        return WavpackGetNumErrors(input.get()) == 0 && (!(dataSize & 1) || output.write("\0", 1) == 1);
    }

}
