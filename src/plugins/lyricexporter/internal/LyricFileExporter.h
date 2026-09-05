// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_LYRIC_EXPORTER_LYRICFILEEXPORTER_H
#define DIFFSCOPE_LYRIC_EXPORTER_LYRICFILEEXPORTER_H

#include <importexportmanager/FileConverter.h>

namespace LyricExporter::Internal {

    class LyricFileExporter : public ImportExportManager::FileConverter {
        Q_OBJECT
    public:
        explicit LyricFileExporter(QObject *parent = nullptr);
        ~LyricFileExporter() override;

        bool execExport(const QString &path, const opendspx::Model &model, QWindow *window) override;
    };

}

#endif // DIFFSCOPE_LYRIC_EXPORTER_LYRICFILEEXPORTER_H
