// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MetadataConverter.h"

#include <QJsonObject>

namespace Synth::Internal::MetadataConverter {

    ArchitectureMetadata architecture(const Api::V1::ArchitectureMetadata &source) {
        ArchitectureMetadata target;
        target.setId(source.id);
        target.setName(source.name);
        switch (source.pronunciationMode) {
            case Api::V1::ArchitectureMetadata::FullPronunciation:
                target.setPronunciationMode(QStringLiteral("FULL"));
                break;
            case Api::V1::ArchitectureMetadata::SkipPronunciation:
                target.setPronunciationMode(QStringLiteral("SKIP"));
                break;
        }
        switch (source.phonemeMode) {
            case Api::V1::ArchitectureMetadata::FullPhoneme:
                target.setPhonemeMode(QStringLiteral("FULL"));
                break;
            case Api::V1::ArchitectureMetadata::TokenOnlyPhoneme:
                target.setPhonemeMode(QStringLiteral("TOKEN_ONLY"));
                break;
            case Api::V1::ArchitectureMetadata::SkipPhoneme:
                target.setPhonemeMode(QStringLiteral("SKIP"));
                break;
        }

        QList<ParameterMetadata> parameters;
        parameters.reserve(source.parameters.size());
        for (auto it = source.parameters.cbegin(); it != source.parameters.cend(); ++it) {
            ParameterMetadata parameter;
            parameter.setId(it.key());
            parameter.setKind(it->type == Api::V1::ArchitectureParameterMetadata::Direct
                                  ? ParameterMetadata::Direct
                                  : ParameterMetadata::Indirect);
            parameter.setDependsOn(it->dependsOn);
            parameters.append(parameter);
        }
        target.setParameters(parameters);
        target.setAudioDependencies(source.audioDependencies);
        return target;
    }

    SingerMetadata singer(const Api::V1::SingerInfo &source, const QUuid &serviceId) {
        SingerMetadata target;
        target.setId(source.id);
        target.setArchitectureId(source.arch);
        target.setName(source.name);
        QString mixGroup;
        if (!source.mixGroup.isEmpty()) {
            mixGroup = QStringLiteral("org.diffscope.synth:%1:%2")
                           .arg(serviceId.toString(QUuid::WithoutBraces), source.mixGroup);
        }
        target.setMixGroup(mixGroup);
        SingerMetadata::LanguageMap languages;
        for (auto it = source.languages.cbegin(); it != source.languages.cend(); ++it) {
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
                {QStringLiteral("audio_url"), item.audioUrl},
            });
        }
        return result;
    }

}
