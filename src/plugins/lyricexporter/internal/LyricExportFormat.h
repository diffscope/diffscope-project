// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_LYRIC_EXPORTER_LYRICEXPORTFORMAT_H
#define DIFFSCOPE_LYRIC_EXPORTER_LYRICEXPORTFORMAT_H

#include <span>

#include <QByteArray>
#include <QList>
#include <QString>

namespace LyricExporter::Internal {

    struct LyricExportRow {
        int startMsec{};
        int endMsec{};
        QString lyric;
    };

    enum class LyricExportFormatType {
        PlainText,
        StartTime,
        StartEndTime,
    };

    using LyricExportSerializer = QByteArray (*)(const QList<LyricExportRow> &rows);

    struct LyricExportFormat {
        LyricExportFormatType type;
        const char *name;
        const char *suffix;
        const char *fileDialogFilter;
        const char *heuristicFilter;
        bool supportsInterludes;
        LyricExportSerializer serialize;

        constexpr int lyricColumn() const {
            switch (type) {
                case LyricExportFormatType::PlainText:
                    return 0;
                case LyricExportFormatType::StartTime:
                    return 1;
                case LyricExportFormatType::StartEndTime:
                    return 2;
            }
            return 0;
        }

        constexpr int columnCount() const {
            return lyricColumn() + 1;
        }
    };

    std::span<const LyricExportFormat> lyricExportFormats();

}

#endif // DIFFSCOPE_LYRIC_EXPORTER_LYRICEXPORTFORMAT_H
