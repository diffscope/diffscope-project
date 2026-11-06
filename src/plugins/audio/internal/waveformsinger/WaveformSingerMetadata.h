// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_WAVEFORMSINGERMETADATA_H
#define DIFFSCOPE_AUDIO_WAVEFORMSINGERMETADATA_H

#include <QCoreApplication>
#include <QString>

#include <singercatalog/ArchitectureInfo.h>
#include <singercatalog/SingerInfo.h>

namespace SingerCatalog {
    class SingerRegistry;
}

namespace Audio::Internal {

    class WaveformSingerMetadata {
        Q_DECLARE_TR_FUNCTIONS(Audio::Internal::WaveformSingerMetadata)

    public:
        static QString architectureId();
        static SingerCatalog::ArchitectureInfo architectureInfo();
        static SingerCatalog::SingerInfo singerInfo();

        static bool registerAll(SingerCatalog::SingerRegistry *registry);
    };

}

#endif // DIFFSCOPE_AUDIO_WAVEFORMSINGERMETADATA_H
