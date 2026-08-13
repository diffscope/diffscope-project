#ifndef DIFFSCOPE_SYNTH_SYNTHESISTASKCODEC_H
#define DIFFSCOPE_SYNTH_SYNTHESISTASKCODEC_H

#include <QByteArray>
#include <QJsonObject>

#include <synth/SynthesisModel.h>
#include <synth/internal/Dtos.h>

namespace Synth::Internal::TaskCodec {

    QString typeName(SynthesisTaskType type);
    QJsonObject contextToJson(const SynthesisContext &context);
    QJsonObject parameterToJson(const SynthesisParameter &parameter);
    QJsonObject scoreCommonToJson(const SynthesisScore &score);
    QJsonObject requestToJson(const SynthesisTaskRequest &request);
    QJsonObject resultToJson(const SynthesisTaskResult &result);
    bool resultFromJson(const QJsonObject &object, SynthesisTaskResult *result);
    QByteArray digest(const QJsonObject &object);

    Api::V1::MultiSingerContext multiContext(const SynthesisContext &context);
    Api::V1::SingleSingerContext singleContext(const SynthesisContext &context);
    QList<Api::V1::ParameterNote> parameterNotes(const SynthesisScore &score);
    Api::V1::Mix mixToDto(const QList<QList<double>> &mix);

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISTASKCODEC_H
