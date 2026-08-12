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

    struct ApplicationInfo {
        Q_GADGET
    public:
        int apiVersion{};

        SYNTH_DSSP_JSON_MEMBERS(ApplicationInfo);
    };

    struct ApplicationInfoResponse {
        Q_GADGET
    public:
        ApplicationInfo dssp;

        SYNTH_DSSP_JSON_MEMBERS(ApplicationInfoResponse);
    };

    struct ArchitectureParameterMetadata {
        Q_GADGET
    public:
        enum Type {
            Direct,
            Indirect,
        };
        Q_ENUM(Type)

        Type type{Direct};
        QStringList dependsOn;

        SYNTH_DSSP_JSON_MEMBERS(ArchitectureParameterMetadata);
    };

    struct ArchitectureMetadata {
        Q_GADGET
    public:
        enum PronunciationMode {
            FullPronunciation,
            SkipPronunciation,
        };
        Q_ENUM(PronunciationMode)

        enum PhonemeMode {
            FullPhoneme,
            TokenOnlyPhoneme,
            SkipPhoneme,
        };
        Q_ENUM(PhonemeMode)

        QString id;
        QString name;
        PronunciationMode pronunciationMode{FullPronunciation};
        PhonemeMode phonemeMode{FullPhoneme};
        QMap<QString, ArchitectureParameterMetadata> parameters;
        QStringList audioDependencies;

        SYNTH_DSSP_JSON_MEMBERS(ArchitectureMetadata);
    };

    struct ArchitectureMetadataList {
        Q_GADGET
    public:
        QList<ArchitectureMetadata> items;

        SYNTH_DSSP_JSON_MEMBERS(ArchitectureMetadataList);
    };

    struct SingerLanguageInfo {
        Q_GADGET
    public:
        QString name;
        QString defaultLyric;

        SYNTH_DSSP_JSON_MEMBERS(SingerLanguageInfo);
    };

    struct SingerInfo {
        Q_GADGET
    public:
        QString id;
        QString name;
        QString arch;
        QString mixGroup;
        QMap<QString, SingerLanguageInfo> languages;
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

#undef SYNTH_DSSP_JSON_MEMBERS

} // namespace Synth::Internal::Api::V1

#endif // DIFFSCOPE_SYNTH_INTERNAL_DTOS_H
