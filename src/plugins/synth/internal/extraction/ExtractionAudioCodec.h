// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_EXTRACTIONAUDIOCODEC_H
#define DIFFSCOPE_SYNTH_EXTRACTIONAUDIOCODEC_H

#include <atomic>
#include <memory>

#include <QByteArray>
#include <QString>
#include <QVariant>

#include <synth/internal/Dtos.h>

namespace Synth::Internal::ExtractionAudioCodec {

    struct Input {
        QString filePath;
        QVariant userData;
        QString formatEntryClassName;
        double offsetSeconds{};
        double durationSeconds{};
    };

    struct Result {
        QByteArray bytes;
        QString mimeType;
        QString error;
    };

    Result encode(const Input &input, const Api::V1::ExtractorInfo &extractor,
                  const std::shared_ptr<std::atomic_bool> &cancelled);
    Result decode(const QByteArray &bytes, const std::shared_ptr<std::atomic_bool> &cancelled);

}

#endif // DIFFSCOPE_SYNTH_EXTRACTIONAUDIOCODEC_H
