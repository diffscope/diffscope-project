// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_AUDIOEXPORTERCONFIG_P_H
#define DIFFSCOPE_AUDIO_AUDIOEXPORTERCONFIG_P_H

#include <audio/AudioExporterConfig.h>

#include <QSharedData>

namespace Audio {

    class AudioExporterConfigData : public QSharedData {
    public:
        QString fileName;
        QString fileDirectory;
        AudioExporterConfig::FileType fileType{};
        bool formatMono{};
        int formatOption{};
        int formatQuality{};
        double formatSampleRate{};
        AudioExporterConfig::MixingOption mixingOption{};
        bool isMuteSoloEnabled{};
        bool isEffectsEnabled{};
        AudioExporterConfig::SourceOption sourceOption{};
        AudioExporterConfig::TimeRange timeRange{};
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOEXPORTERCONFIG_P_H
