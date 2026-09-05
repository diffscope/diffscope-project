// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "LyricExportFormat.h"

#include <array>

#include <QChar>
#include <QCoreApplication>
#include <QString>

#include <SVSCraftCore/LongTime.h>

namespace LyricExporter::Internal {

    static QString formatLrcTime(int msec) {
        return SVS::LongTime(msec).toString(2, 2, 2);
    }

    static QString formatSrtTime(int msec) {
        const qint64 total = msec;
        const qint64 hours = total / 3600000;
        const qint64 minutes = total % 3600000 / 60000;
        const qint64 seconds = total % 60000 / 1000;
        const qint64 milliseconds = total % 1000;
        return QStringLiteral("%1:%2:%3,%4")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'))
            .arg(milliseconds, 3, 10, QLatin1Char('0'));
    }

    static QString escapeCsvField(QString field) {
        if (!field.contains(QLatin1Char(',')) && !field.contains(QLatin1Char('"'))
            && !field.contains(QLatin1Char('\r')) && !field.contains(QLatin1Char('\n'))) {
            return field;
        }
        field.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + field + QLatin1Char('"');
    }

    static QByteArray serializeLrc(const QList<LyricExportRow> &rows) {
        QString output;
        for (const auto &row : rows) {
            output += QLatin1Char('[') + formatLrcTime(row.startMsec)
                + QLatin1Char(']') + row.lyric + QLatin1Char('\n');
        }
        return output.toUtf8();
    }

    static QByteArray serializeSrt(const QList<LyricExportRow> &rows) {
        QString output;
        for (qsizetype i = 0; i < rows.size(); ++i) {
            const auto &row = rows.at(i);
            output += QString::number(i + 1) + QStringLiteral("\r\n")
                + formatSrtTime(row.startMsec) + QStringLiteral(" --> ")
                + formatSrtTime(row.endMsec) + QStringLiteral("\r\n")
                + row.lyric + QStringLiteral("\r\n\r\n");
        }
        return output.toUtf8();
    }

    static QByteArray serializeCsv(const QList<LyricExportRow> &rows) {
        QString output = QStringLiteral("Start,End,Lyrics\r\n");
        for (const auto &row : rows) {
            output += escapeCsvField(SVS::LongTime(row.startMsec).toString())
                + QLatin1Char(',')
                + escapeCsvField(SVS::LongTime(row.endMsec).toString())
                + QLatin1Char(',') + escapeCsvField(row.lyric)
                + QStringLiteral("\r\n");
        }
        return output.toUtf8();
    }

    static QByteArray serializePlainText(const QList<LyricExportRow> &rows) {
        QString output;
        for (const auto &row : rows)
            output += row.lyric + QLatin1Char('\n');
        return output.toUtf8();
    }

    static constexpr std::array<LyricExportFormat, 4> kLyricExportFormats{{
        {
            LyricExportFormatType::StartTime,
            "LRC",
            "lrc",
            QT_TRANSLATE_NOOP("LyricExporter::Internal::LyricFileExporter", "LRC Lyrics (*.lrc)"),
            "*.lrc",
            true,
            &serializeLrc,
        },
        {
            LyricExportFormatType::StartEndTime,
            "SRT",
            "srt",
            QT_TRANSLATE_NOOP("LyricExporter::Internal::LyricFileExporter", "SubRip Subtitle (*.srt)"),
            "*.srt",
            false,
            &serializeSrt,
        },
        {
            LyricExportFormatType::StartEndTime,
            "CSV",
            "csv",
            QT_TRANSLATE_NOOP("LyricExporter::Internal::LyricFileExporter", "Comma-Separated Values (*.csv)"),
            "*.csv",
            false,
            &serializeCsv,
        },
        {
            LyricExportFormatType::PlainText,
            "Plain Text",
            "txt",
            QT_TRANSLATE_NOOP("LyricExporter::Internal::LyricFileExporter", "Plain Text (*.txt)"),
            "*.txt",
            false,
            &serializePlainText,
        },
    }};

    std::span<const LyricExportFormat> lyricExportFormats() {
        return kLyricExportFormats;
    }

}
