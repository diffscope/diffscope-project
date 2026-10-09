// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MetadataConverter.h"

#include <QJsonObject>
#include <QUrl>

#include <synth/internal/SynthesisPipeline.h>

namespace Synth::Internal::MetadataConverter {

    ArchitectureMetadata architecture(const Api::V1::ArchitectureMetadata &source) {
        ArchitectureMetadata target;
        target.setId(source.id);
        target.setName(source.name);
        target.setParameters(source.toJson().toObject().value(QStringLiteral("parameters")).toObject());
        return target;
    }

    SingerMetadata singer(const Api::V1::SingerInfo &source, const Api::V1::GroupMetadata &group, const QUuid &serviceId) {
        SingerMetadata target;
        target.setId(source.id);
        target.setArchitectureId(source.arch);
        target.setName(source.name);
        QString mixGroup;
        if (group.mixable) {
            mixGroup = QStringLiteral("org.diffscope.synth:%1:%2:%3")
                           .arg(serviceId.toString(QUuid::WithoutBraces), QString::fromLatin1(QUrl::toPercentEncoding(source.arch)), QString::fromLatin1(QUrl::toPercentEncoding(group.id)));
        }
        target.setMixGroup(mixGroup);
        SynthesisMetadataAccess::setGroupId(target, group.id);
        target.setSupportedParameters(group.parameterPipeline.keys());
        SingerMetadata::LanguageMap languages;
        for (auto it = group.languages.cbegin(); it != group.languages.cend(); ++it) {
            SingerLanguageMetadata language;
            language.name = it->name;
            language.defaultLyric = it->defaultLyric;
            languages.insert(it.key(), language);
        }
        target.setLanguages(languages);
        target.setDefaultLanguage(source.defaultLanguage);
        target.setArchitectureSpecificInfo(source.archSpecificInfo);
        target.setDefaultExtra(source.defaultExtra);
        return target;
    }

    QJsonArray demos(const Api::V1::SingerDemoAudioList &source) {
        QJsonArray result;
        for (const auto &item : source.items) {
            result.append(QJsonObject{
                {QStringLiteral("name"), item.name},
                {QStringLiteral("audioUrl"), item.audioUrl},
            });
        }
        return result;
    }

}
