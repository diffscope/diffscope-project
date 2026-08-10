#ifndef DIFFSCOPE_SYNTH_INTERNAL_METADATACONVERTER_H
#define DIFFSCOPE_SYNTH_INTERNAL_METADATACONVERTER_H

#include <synth/ServiceTypes.h>
#include <synth/internal/Dtos.h>

namespace Synth::Internal::MetadataConverter {

    ArchitectureMetadata architecture(const Api::V1::ArchitectureMetadata &source);
    SingerMetadata singer(const Api::V1::SingerInfo &source);
    QJsonArray demos(const Api::V1::SingerDemoAudioList &source);

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_METADATACONVERTER_H
