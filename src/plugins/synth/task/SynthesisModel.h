// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISMODEL_H
#define DIFFSCOPE_SYNTH_SYNTHESISMODEL_H

#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QString>
#include <QStringList>

#include <synth/synthglobal.h>

namespace Synth {

    enum class SynthesisTaskType {
        Pronunciation,
        Phoneme,
        Duration,
        Parameter,
        Audio,
    };

    struct SYNTH_EXPORT SynthesisSinger {
        QString id;
        QJsonValue extra{QJsonValue::Null};

        bool operator==(const SynthesisSinger &) const = default;
    };

    struct SYNTH_EXPORT SynthesisContext {
        QString architectureId;
        QJsonValue architectureExtra{QJsonValue::Null};
        QList<SynthesisSinger> singers;

        bool operator==(const SynthesisContext &) const = default;
    };

    struct SYNTH_EXPORT SynthesisLyricNote {
        QString lyric;
        QString language;

        bool operator==(const SynthesisLyricNote &) const = default;
    };

    struct SYNTH_EXPORT SynthesisPronunciationNote {
        QString pronunciation;
        QString language;

        bool operator==(const SynthesisPronunciationNote &) const = default;
    };

    struct SYNTH_EXPORT SynthesisPhoneme {
        QString token;
        bool onset{};
        QString language;
        double start{};

        bool operator==(const SynthesisPhoneme &) const = default;
    };

    struct SYNTH_EXPORT SynthesisScoreNote {
        double gap{};
        double duration{};
        int cent{};
        QString pronunciation;
        QString language;
        QList<SynthesisPhoneme> phonemes;

        bool operator==(const SynthesisScoreNote &) const = default;
    };

    struct SYNTH_EXPORT SynthesisParameter {
        QList<double> values;
        double sampleRate{100.0};

        bool operator==(const SynthesisParameter &) const = default;
    };

    struct SYNTH_EXPORT SynthesisScore {
        double pieceDuration{};
        QList<SynthesisScoreNote> notes;
        QList<QList<double>> mix;
        double mixSampleRate{100.0};
        QMap<QString, SynthesisParameter> parameters;
        QStringList requestedParameters;

        bool operator==(const SynthesisScore &) const = default;
    };

    struct SYNTH_EXPORT SynthesisTaskRequest {
        SynthesisTaskType type{SynthesisTaskType::Pronunciation};
        SynthesisContext context;
        QList<SynthesisLyricNote> lyricNotes;
        QList<SynthesisPronunciationNote> pronunciationNotes;
        SynthesisScore score;
        QString displayName;

        bool operator==(const SynthesisTaskRequest &) const = default;
    };

    struct SYNTH_EXPORT SynthesisTaskOptions {
        bool readCache{true};
        bool writeCache{true};
        int priority{};

        bool operator==(const SynthesisTaskOptions &) const = default;
    };

    struct SYNTH_EXPORT SynthesisPronunciationResult {
        QString pronunciation;
        QStringList candidates;

        bool operator==(const SynthesisPronunciationResult &) const = default;
    };

    struct SYNTH_EXPORT SynthesisTaskResult {
        QList<SynthesisPronunciationResult> pronunciations;
        QList<QList<SynthesisPhoneme>> phonemes;
        QMap<QString, SynthesisParameter> parameters;
        QString audioFilePath;
        bool fromCache{};

        bool operator==(const SynthesisTaskResult &) const = default;
    };

}

Q_DECLARE_METATYPE(Synth::SynthesisTaskType)
Q_DECLARE_METATYPE(Synth::SynthesisSinger)
Q_DECLARE_METATYPE(Synth::SynthesisContext)
Q_DECLARE_METATYPE(Synth::SynthesisLyricNote)
Q_DECLARE_METATYPE(Synth::SynthesisPronunciationNote)
Q_DECLARE_METATYPE(Synth::SynthesisPhoneme)
Q_DECLARE_METATYPE(Synth::SynthesisScoreNote)
Q_DECLARE_METATYPE(Synth::SynthesisParameter)
Q_DECLARE_METATYPE(Synth::SynthesisScore)
Q_DECLARE_METATYPE(Synth::SynthesisTaskRequest)
Q_DECLARE_METATYPE(Synth::SynthesisTaskOptions)
Q_DECLARE_METATYPE(Synth::SynthesisTaskResult)

#endif // DIFFSCOPE_SYNTH_SYNTHESISMODEL_H
