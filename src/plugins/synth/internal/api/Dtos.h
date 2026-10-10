// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_DTOS_H
#define DIFFSCOPE_SYNTH_INTERNAL_DTOS_H

#include <optional>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QString>
#include <QStringList>

namespace Synth::Internal::Api::V1 {

#define SYNTH_DSSP_JSON_MEMBERS(Type)                                                                                  \
    QJsonValue toJson() const;                                                                                         \
    static bool fromJson(const QJsonValue &json, Type &value, QString *errorMessage = nullptr)

    struct ApplicationApiVersion {
        Q_GADGET
    public:
        double major{};
        double minor{};

        SYNTH_DSSP_JSON_MEMBERS(ApplicationApiVersion);
    };

    struct ApplicationInfo {
        Q_GADGET
    public:
        ApplicationApiVersion apiVersion;

        SYNTH_DSSP_JSON_MEMBERS(ApplicationInfo);
    };

    struct ApplicationInfoResponse {
        Q_GADGET
    public:
        ApplicationInfo dssp;

        SYNTH_DSSP_JSON_MEMBERS(ApplicationInfoResponse);
    };

    struct ParameterDefinition {
        Q_GADGET
    public:
        QString name;
        double defaultValue{};
        bool showBaseline{};
        double baselineValue{};
        QString fillMode;
        bool fallbackOnDefaultValue{};
        QJsonValue displayValueMappingExpression;
        QJsonValue displayValueInverseMappingExpression;
        QString displayValuePrefix;
        QString displayValueSuffix;
        int displayValueDecimalPlaces{};

        SYNTH_DSSP_JSON_MEMBERS(ParameterDefinition);
    };

    struct ParameterPipelineMetadata {
        Q_GADGET
    public:
        enum Type { Direct, Indirect };
        Q_ENUM(Type)

        Type type{Direct};
        QStringList dependsOn;
        QString retakeMode;

        SYNTH_DSSP_JSON_MEMBERS(ParameterPipelineMetadata);
    };

    struct ArchitectureMetadata {
        Q_GADGET
    public:
        QString id;
        QString name;
        QMap<QString, ParameterDefinition> parameters;

        SYNTH_DSSP_JSON_MEMBERS(ArchitectureMetadata);
    };

    struct ArchitectureMetadataList {
        Q_GADGET
    public:
        QList<ArchitectureMetadata> items;

        SYNTH_DSSP_JSON_MEMBERS(ArchitectureMetadataList);
    };

    struct GroupLanguageInfo {
        Q_GADGET
    public:
        QString name;
        QString defaultLyric;
        QString pronunciationMode;
        QString phonemeMode;

        SYNTH_DSSP_JSON_MEMBERS(GroupLanguageInfo);
    };

    struct GroupMetadata {
        Q_GADGET
    public:
        QString id;
        QString arch;
        QMap<QString, GroupLanguageInfo> languages;
        QString durationMode;
        QMap<QString, ParameterPipelineMetadata> parameterPipeline;
        QStringList audioDependencies;
        bool mixable{};
        QJsonValue archSpecificInfo;
        QJsonValue defaultArchExtra;

        SYNTH_DSSP_JSON_MEMBERS(GroupMetadata);
    };

    struct GroupMetadataList {
        Q_GADGET
    public:
        QList<GroupMetadata> items;

        SYNTH_DSSP_JSON_MEMBERS(GroupMetadataList);
    };

    struct SingerInfo {
        Q_GADGET
    public:
        QString id;
        QString name;
        QString arch;
        QString group;
        QString defaultLanguage;
        QJsonValue archSpecificInfo;
        QJsonValue defaultExtra;

        SYNTH_DSSP_JSON_MEMBERS(SingerInfo);
    };

    struct SingerInfoList {
        Q_GADGET
    public:
        QList<SingerInfo> items;

        SYNTH_DSSP_JSON_MEMBERS(SingerInfoList);
    };

    struct SingerAvatarResponse {
        Q_GADGET
    public:
        QString avatarUrl;

        SYNTH_DSSP_JSON_MEMBERS(SingerAvatarResponse);
    };

    struct SingerBackgroundResponse {
        Q_GADGET
    public:
        QString backgroundUrl;

        SYNTH_DSSP_JSON_MEMBERS(SingerBackgroundResponse);
    };

    struct SingerDemoAudio {
        Q_GADGET
    public:
        QString name;
        QString audioUrl;

        SYNTH_DSSP_JSON_MEMBERS(SingerDemoAudio);
    };

    struct SingerDemoAudioList {
        Q_GADGET
    public:
        QList<SingerDemoAudio> items;

        SYNTH_DSSP_JSON_MEMBERS(SingerDemoAudioList);
    };

    struct Singer {
        Q_GADGET
    public:
        QString id;
        QJsonValue extra;

        SYNTH_DSSP_JSON_MEMBERS(Singer);
    };

    struct SingleSingerContext {
        Q_GADGET
    public:
        QString arch;
        QJsonValue archExtra;
        Singer singer;

        SYNTH_DSSP_JSON_MEMBERS(SingleSingerContext);
    };

    struct MultiSingerContext {
        Q_GADGET
    public:
        QString arch;
        QJsonValue archExtra;
        QList<Singer> singers;

        SYNTH_DSSP_JSON_MEMBERS(MultiSingerContext);
    };

    struct EnvTagRequest {
        Q_GADGET
    public:
        MultiSingerContext context;

        SYNTH_DSSP_JSON_MEMBERS(EnvTagRequest);
    };

    struct EnvTagResponse {
        Q_GADGET
    public:
        QString envTag;

        SYNTH_DSSP_JSON_MEMBERS(EnvTagResponse);
    };

    struct Lyric {
        Q_GADGET
    public:
        QString lyric;
        QString language;

        SYNTH_DSSP_JSON_MEMBERS(Lyric);
    };

    struct PronunciationInput {
        Q_GADGET
    public:
        QList<Lyric> notes;

        SYNTH_DSSP_JSON_MEMBERS(PronunciationInput);
    };

    struct PronunciationRequest {
        Q_GADGET
    public:
        SingleSingerContext context;
        PronunciationInput input;

        SYNTH_DSSP_JSON_MEMBERS(PronunciationRequest);
    };

    struct PronunciationNote {
        Q_GADGET
    public:
        QString pronunciation;
        QString language;
        std::optional<int> syllableSliceStart;
        std::optional<int> syllableSliceEnd;

        SYNTH_DSSP_JSON_MEMBERS(PronunciationNote);
    };

    struct PhonemeInput {
        Q_GADGET
    public:
        QList<PronunciationNote> notes;

        SYNTH_DSSP_JSON_MEMBERS(PhonemeInput);
    };

    struct PhonemeRequest {
        Q_GADGET
    public:
        SingleSingerContext context;
        PhonemeInput input;

        SYNTH_DSSP_JSON_MEMBERS(PhonemeRequest);
    };

    struct NotePosition {
        Q_GADGET
    public:
        double gap{};
        double duration{};

        SYNTH_DSSP_JSON_MEMBERS(NotePosition);
    };

    struct DurationInputPhoneme {
        Q_GADGET
    public:
        QString token;
        bool onset{};
        QString language;

        SYNTH_DSSP_JSON_MEMBERS(DurationInputPhoneme);
    };

    struct DurationNote {
        Q_GADGET
    public:
        NotePosition position;
        int cent{};
        QString kind{QStringLiteral("normal")};
        QString pronunciation;
        QString language;
        QList<DurationInputPhoneme> phonemes;

        SYNTH_DSSP_JSON_MEMBERS(DurationNote);
    };

    struct Mix {
        Q_GADGET
    public:
        QList<QList<double>> rows;

        SYNTH_DSSP_JSON_MEMBERS(Mix);
    };

    struct DurationInput {
        Q_GADGET
    public:
        double pieceDuration{};
        QList<DurationNote> notes;
        Mix mix;
        double mixSampleRate{};

        SYNTH_DSSP_JSON_MEMBERS(DurationInput);
    };

    struct DurationRequest {
        Q_GADGET
    public:
        MultiSingerContext context;
        DurationInput input;

        SYNTH_DSSP_JSON_MEMBERS(DurationRequest);
    };

    struct ParameterInputPhoneme {
        Q_GADGET
    public:
        QString token;
        bool onset{};
        QString language;
        double start{};

        SYNTH_DSSP_JSON_MEMBERS(ParameterInputPhoneme);
    };

    struct ParameterNote {
        Q_GADGET
    public:
        NotePosition position;
        int cent{};
        QString kind{QStringLiteral("normal")};
        QString pronunciation;
        QString language;
        QList<ParameterInputPhoneme> phonemes;

        SYNTH_DSSP_JSON_MEMBERS(ParameterNote);
    };

    struct ParameterRetake {
        Q_GADGET
    public:
        int position{};
        int length{};

        SYNTH_DSSP_JSON_MEMBERS(ParameterRetake);
    };

    struct Parameter {
        Q_GADGET
    public:
        QList<double> values;
        double sampleRate{};
        std::optional<ParameterRetake> retake;

        SYNTH_DSSP_JSON_MEMBERS(Parameter);
    };

    struct AudioParameter {
        Q_GADGET
    public:
        std::optional<QList<double>> values;
        double sampleRate{};

        SYNTH_DSSP_JSON_MEMBERS(AudioParameter);
    };

    struct ParameterMap {
        Q_GADGET
    public:
        QMap<QString, Parameter> values;

        SYNTH_DSSP_JSON_MEMBERS(ParameterMap);
    };

    struct AudioParameterMap {
        Q_GADGET
    public:
        QMap<QString, AudioParameter> values;

        SYNTH_DSSP_JSON_MEMBERS(AudioParameterMap);
    };

    struct ParameterInput {
        Q_GADGET
    public:
        double pieceDuration{};
        QList<ParameterNote> notes;
        Mix mix;
        double mixSampleRate{};
        ParameterMap parameters;

        SYNTH_DSSP_JSON_MEMBERS(ParameterInput);
    };

    struct ParameterRequest {
        Q_GADGET
    public:
        MultiSingerContext context;
        ParameterInput input;

        SYNTH_DSSP_JSON_MEMBERS(ParameterRequest);
    };

    struct AudioInput {
        Q_GADGET
    public:
        double pieceDuration{};
        QList<ParameterNote> notes;
        Mix mix;
        double mixSampleRate{};
        AudioParameterMap parameters;

        SYNTH_DSSP_JSON_MEMBERS(AudioInput);
    };

    struct AudioRequest {
        Q_GADGET
    public:
        MultiSingerContext context;
        AudioInput input;
        QStringList acceptableFormats;
        QStringList acceptableSchemes{QStringLiteral("data"), QStringLiteral("http"), QStringLiteral("https")};

        SYNTH_DSSP_JSON_MEMBERS(AudioRequest);
    };

    struct Pronunciation {
        Q_GADGET
    public:
        QString pronunciation;
        QStringList candidates;

        SYNTH_DSSP_JSON_MEMBERS(Pronunciation);
    };

    struct PronunciationOutput {
        Q_GADGET
    public:
        QList<Pronunciation> notes;

        SYNTH_DSSP_JSON_MEMBERS(PronunciationOutput);
    };

    struct PronunciationResponse {
        Q_GADGET
    public:
        PronunciationOutput output;

        SYNTH_DSSP_JSON_MEMBERS(PronunciationResponse);
    };

    struct Phoneme {
        Q_GADGET
    public:
        QString token;
        bool onset{};

        SYNTH_DSSP_JSON_MEMBERS(Phoneme);
    };

    struct PhonemeNote {
        Q_GADGET
    public:
        QList<Phoneme> phonemes;

        SYNTH_DSSP_JSON_MEMBERS(PhonemeNote);
    };

    struct PhonemeOutput {
        Q_GADGET
    public:
        QList<PhonemeNote> notes;

        SYNTH_DSSP_JSON_MEMBERS(PhonemeOutput);
    };

    struct PhonemeResponse {
        Q_GADGET
    public:
        PhonemeOutput output;

        SYNTH_DSSP_JSON_MEMBERS(PhonemeResponse);
    };

    struct DurationOutputPhoneme {
        Q_GADGET
    public:
        double start{};

        SYNTH_DSSP_JSON_MEMBERS(DurationOutputPhoneme);
    };

    struct DurationOutputNote {
        Q_GADGET
    public:
        QList<DurationOutputPhoneme> phonemes;

        SYNTH_DSSP_JSON_MEMBERS(DurationOutputNote);
    };

    struct DurationOutput {
        Q_GADGET
    public:
        QList<DurationOutputNote> notes;

        SYNTH_DSSP_JSON_MEMBERS(DurationOutput);
    };

    struct DurationResponse {
        Q_GADGET
    public:
        DurationOutput output;

        SYNTH_DSSP_JSON_MEMBERS(DurationResponse);
    };

    struct ParameterOutputParameter {
        Q_GADGET
    public:
        QList<double> values;
        double sampleRate{};

        SYNTH_DSSP_JSON_MEMBERS(ParameterOutputParameter);
    };

    struct ParameterOutput {
        Q_GADGET
    public:
        QMap<QString, ParameterOutputParameter> parameters;

        SYNTH_DSSP_JSON_MEMBERS(ParameterOutput);
    };

    struct ParameterResponse {
        Q_GADGET
    public:
        ParameterOutput output;

        SYNTH_DSSP_JSON_MEMBERS(ParameterResponse);
    };

    struct AudioOutput {
        Q_GADGET
    public:
        QString audioUrl;

        SYNTH_DSSP_JSON_MEMBERS(AudioOutput);
    };

    struct AudioResponse {
        Q_GADGET
    public:
        AudioOutput output;

        SYNTH_DSSP_JSON_MEMBERS(AudioResponse);
    };

    struct ExtractorInfo {
        Q_GADGET
    public:
        QString id;
        QString name;
        int preferredAudioSampleRate{};
        QStringList acceptableFormats;
        QStringList acceptableSchemes;
        QList<int> acceptableAudioSampleRates;

        SYNTH_DSSP_JSON_MEMBERS(ExtractorInfo);
    };

    struct SeparationTrackInfo {
        Q_GADGET
    public:
        QString name;

        SYNTH_DSSP_JSON_MEMBERS(SeparationTrackInfo);
    };

    struct SeparationExtractorInfo : ExtractorInfo {
        Q_GADGET
    public:
        QList<SeparationTrackInfo> tracks;

        SYNTH_DSSP_JSON_MEMBERS(SeparationExtractorInfo);
    };

    struct ExtractorList {
        Q_GADGET
    public:
        QList<ExtractorInfo> note;
        QList<ExtractorInfo> tempo;
        QList<ExtractorInfo> pitch;
        QList<SeparationExtractorInfo> separation;

        SYNTH_DSSP_JSON_MEMBERS(ExtractorList);
    };

    struct ExtractionInput {
        Q_GADGET
    public:
        QString audioUrl;

        SYNTH_DSSP_JSON_MEMBERS(ExtractionInput);
    };

    struct ExtractionRequest {
        Q_GADGET
    public:
        QString extractor;
        ExtractionInput input;

        SYNTH_DSSP_JSON_MEMBERS(ExtractionRequest);
    };

    using NoteExtractionRequest = ExtractionRequest;
    using TempoExtractionRequest = ExtractionRequest;
    using PitchExtractionRequest = ExtractionRequest;

    struct SeparationExtractionRequest {
        Q_GADGET
    public:
        QString extractor;
        ExtractionInput input;
        QStringList acceptableFormats;
        QStringList acceptableSchemes{QStringLiteral("data"), QStringLiteral("http"), QStringLiteral("https")};

        SYNTH_DSSP_JSON_MEMBERS(SeparationExtractionRequest);
    };

    struct ExtractedNote {
        Q_GADGET
    public:
        NotePosition position;
        int cent{};

        SYNTH_DSSP_JSON_MEMBERS(ExtractedNote);
    };

    struct NoteExtractionOutput {
        Q_GADGET
    public:
        QList<ExtractedNote> notes;

        SYNTH_DSSP_JSON_MEMBERS(NoteExtractionOutput);
    };

    struct NoteExtractionResponse {
        Q_GADGET
    public:
        NoteExtractionOutput output;

        SYNTH_DSSP_JSON_MEMBERS(NoteExtractionResponse);
    };

    struct ExtractedBeat {
        Q_GADGET
    public:
        double position{};
        bool downbeat{};

        SYNTH_DSSP_JSON_MEMBERS(ExtractedBeat);
    };

    struct TempoExtractionOutput {
        Q_GADGET
    public:
        QList<ExtractedBeat> beats;

        SYNTH_DSSP_JSON_MEMBERS(TempoExtractionOutput);
    };

    struct TempoExtractionResponse {
        Q_GADGET
    public:
        TempoExtractionOutput output;

        SYNTH_DSSP_JSON_MEMBERS(TempoExtractionResponse);
    };

    struct PitchExtractionSegment {
        Q_GADGET
    public:
        int gap{};
        QList<double> pitch;

        SYNTH_DSSP_JSON_MEMBERS(PitchExtractionSegment);
    };

    struct PitchExtractionOutput {
        Q_GADGET
    public:
        QList<PitchExtractionSegment> segments;
        double sampleRate{};

        SYNTH_DSSP_JSON_MEMBERS(PitchExtractionOutput);
    };

    struct PitchExtractionResponse {
        Q_GADGET
    public:
        PitchExtractionOutput output;

        SYNTH_DSSP_JSON_MEMBERS(PitchExtractionResponse);
    };

    struct SeparationTrack {
        Q_GADGET
    public:
        QString name;
        QString audioUrl;

        SYNTH_DSSP_JSON_MEMBERS(SeparationTrack);
    };

    struct SeparationExtractionOutput {
        Q_GADGET
    public:
        QList<SeparationTrack> tracks;

        SYNTH_DSSP_JSON_MEMBERS(SeparationExtractionOutput);
    };

    struct SeparationExtractionResponse {
        Q_GADGET
    public:
        SeparationExtractionOutput output;

        SYNTH_DSSP_JSON_MEMBERS(SeparationExtractionResponse);
    };

#undef SYNTH_DSSP_JSON_MEMBERS

} // namespace Synth::Internal::Api::V1

#endif // DIFFSCOPE_SYNTH_INTERNAL_DTOS_H
