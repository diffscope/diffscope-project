// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisPipeline.h"

#include <algorithm>
#include <ranges>

#include <synth/private/ServiceTypes_p.h>

namespace Synth::Internal {

    QList<Api::V1::GroupMetadata> SynthesisMetadataAccess::groups(const ServiceMetadata &metadata) {
        return metadata.d->groups;
    }

    void SynthesisMetadataAccess::setGroups(ServiceMetadata &metadata, const QList<Api::V1::GroupMetadata> &groups) {
        metadata.d->groups = groups;
    }

    QString SynthesisMetadataAccess::groupId(const SingerMetadata &singer) {
        return singer.d->groupId;
    }

    void SynthesisMetadataAccess::setGroupId(SingerMetadata &singer, const QString &groupId) {
        singer.d->groupId = groupId;
    }

    std::optional<SynthesisPipeline> SynthesisMetadataAccess::resolve(const ServiceMetadata &metadata, const SynthesisContext &context) {
        if (context.singers.isEmpty())
            return std::nullopt;
        const auto architectures = metadata.architectures();
        const auto architecture = std::ranges::find(architectures, context.architectureId, &ArchitectureMetadata::id);
        if (architecture == architectures.end())
            return std::nullopt;
        const auto singers = metadata.singers();
        QString selectedGroup;
        QString defaultLanguage;
        for (qsizetype index = 0; index < context.singers.size(); ++index) {
            const auto singer = std::ranges::find_if(singers, [&](const SingerMetadata &candidate) {
                return candidate.architectureId() == context.architectureId && candidate.id() == context.singers.at(index).id;
            });
            if (singer == singers.end())
                return std::nullopt;
            if (index == 0) {
                selectedGroup = groupId(*singer);
                defaultLanguage = singer->defaultLanguage();
            } else if (groupId(*singer) != selectedGroup) {
                return std::nullopt;
            }
        }
        for (const auto &group : groups(metadata)) {
            if (group.arch == context.architectureId && group.id == selectedGroup) {
                if (context.singers.size() > 1 && !group.mixable)
                    return std::nullopt;
                return SynthesisPipeline{*architecture, group, defaultLanguage};
            }
        }
        return std::nullopt;
    }

}
