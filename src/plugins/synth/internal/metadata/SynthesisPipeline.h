// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_SYNTHESISPIPELINE_H
#define DIFFSCOPE_SYNTH_INTERNAL_SYNTHESISPIPELINE_H

#include <optional>

#include <synth/ServiceTypes.h>
#include <synth/SynthesisModel.h>
#include <synth/internal/Dtos.h>

namespace Synth::Internal {

    struct SynthesisPipeline {
        ArchitectureMetadata architecture;
        Api::V1::GroupMetadata group;
        QString defaultLanguage;
    };

    struct SynthesisMetadataAccess {
        static QList<Api::V1::GroupMetadata> groups(const ServiceMetadata &metadata);
        static void setGroups(ServiceMetadata &metadata, const QList<Api::V1::GroupMetadata> &groups);
        static QString groupId(const SingerMetadata &singer);
        static void setGroupId(SingerMetadata &singer, const QString &groupId);
        static std::optional<SynthesisPipeline> resolve(const ServiceMetadata &metadata, const SynthesisContext &context);
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_SYNTHESISPIPELINE_H
