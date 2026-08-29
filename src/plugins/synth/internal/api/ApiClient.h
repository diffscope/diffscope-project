// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_APICLIENT_H
#define DIFFSCOPE_SYNTH_INTERNAL_APICLIENT_H

#include <memory>

#include <QFuture>
#include <QObject>

#include <synth/ServiceTypes.h>
#include <synth/internal/ApiTypes.h>
#include <synth/internal/Dtos.h>

namespace Synth::Internal::Api {

    class ApiClient : public QObject {
        Q_OBJECT
    public:
        explicit ApiClient(QObject *parent = nullptr);
        ~ApiClient() override;

        QFuture<ApiResult<V1::ApplicationInfoResponse>>
            getInfo(const ServiceInstanceConfiguration &service);
        QFuture<ApiResult<V1::ArchitectureMetadataList>>
            getArchitectures(const ServiceInstanceConfiguration &service,
                             const QString &displayLanguage = {});
        QFuture<ApiResult<V1::ArchitectureMetadata>>
            getArchitecture(const ServiceInstanceConfiguration &service, const QString &architectureId,
                            const QString &displayLanguage = {});
        QFuture<ApiResult<V1::SingerInfoList>>
            getSingers(const ServiceInstanceConfiguration &service,
                       const QString &displayLanguage = {});
        QFuture<ApiResult<V1::SingerInfoList>>
            getArchitectureSingers(const ServiceInstanceConfiguration &service,
                                   const QString &architectureId,
                                   const QString &displayLanguage = {});
        QFuture<ApiResult<V1::SingerInfo>>
            getSinger(const ServiceInstanceConfiguration &service, const QString &architectureId,
                      const QString &singerId, const QString &displayLanguage = {});
        QFuture<ApiResult<V1::SingerAvatarResponse>>
            getSingerAvatar(const ServiceInstanceConfiguration &service, const QString &architectureId,
                            const QString &singerId, const QString &displayLanguage = {});
        QFuture<ApiResult<V1::SingerBackgroundResponse>>
            getSingerBackground(const ServiceInstanceConfiguration &service,
                                const QString &architectureId, const QString &singerId,
                                const QString &displayLanguage = {});
        QFuture<ApiResult<V1::SingerDemoAudioList>>
            getSingerDemoAudio(const ServiceInstanceConfiguration &service,
                               const QString &architectureId, const QString &singerId,
                               const QString &displayLanguage = {});

        QFuture<ApiResult<V1::EnvTagResponse>>
            createEnvironmentTag(const ServiceInstanceConfiguration &service,
                                 const V1::EnvTagRequest &request);
        QFuture<ApiResult<V1::PronunciationResponse>>
            synthesizePronunciation(const ServiceInstanceConfiguration &service,
                                    const V1::PronunciationRequest &request);
        QFuture<ApiResult<V1::PhonemeResponse>>
            synthesizePhoneme(const ServiceInstanceConfiguration &service,
                              const V1::PhonemeRequest &request);
        QFuture<ApiResult<V1::DurationResponse>>
            synthesizeDuration(const ServiceInstanceConfiguration &service,
                               const V1::DurationRequest &request);
        QFuture<ApiResult<V1::ParameterResponse>>
            synthesizeParameter(const ServiceInstanceConfiguration &service,
                                const V1::ParameterRequest &request);
        QFuture<ApiResult<V1::AudioResponse>>
            synthesizeAudio(const ServiceInstanceConfiguration &service,
                            const V1::AudioRequest &request);

        void cancelAll();
        void shutdown();

    private:
        class Private;
        std::unique_ptr<Private> d;
    };

} // namespace Synth::Internal::Api

#endif // DIFFSCOPE_SYNTH_INTERNAL_APICLIENT_H
