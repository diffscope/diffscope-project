// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_LYRIC_EXPORTER_LYRICEXPORTSESSION_H
#define DIFFSCOPE_LYRIC_EXPORTER_LYRICEXPORTSESSION_H

#include <memory>

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QStringList>
#include <qqmlintegration.h>

class QWindow;

namespace opendspx {
    struct Model;
}

namespace LyricExporter::Internal {

    struct LyricExportFormat;
    class LyricExportSessionData;

    class LyricExportSession : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")

        Q_PROPERTY(QAbstractItemModel *trackModel READ trackModel CONSTANT)
        Q_PROPERTY(QAbstractItemModel *lineModel READ lineModel CONSTANT)
        Q_PROPERTY(QAbstractItemModel *tableModel READ tableModel CONSTANT)
        Q_PROPERTY(int selectedTrack READ selectedTrack WRITE setSelectedTrack NOTIFY selectedTrackChanged)
        Q_PROPERTY(QStringList filters READ filters NOTIFY filtersChanged)
        Q_PROPERTY(bool useSpaces READ useSpaces WRITE setUseSpaces NOTIFY useSpacesChanged)
        Q_PROPERTY(int lyricsColumn READ lyricsColumn CONSTANT)
        Q_PROPERTY(int tableRowCount READ tableRowCount NOTIFY tableRowCountChanged)
        Q_PROPERTY(QString validationMessage READ validationMessage NOTIFY validationMessageChanged)
        Q_PROPERTY(ExportState exportState READ exportState NOTIFY exportStateChanged)
        Q_PROPERTY(bool exported READ exported NOTIFY exportStateChanged)

    public:
        enum ExportState {
            Idle,
            Exporting,
            Succeeded,
            Failed,
        };
        Q_ENUM(ExportState)

        explicit LyricExportSession(const LyricExportFormat &format, const QString &path,
                                    const opendspx::Model &model, QWindow *window,
                                    QObject *parent = nullptr);
        ~LyricExportSession() override;

        QAbstractItemModel *trackModel() const;
        QAbstractItemModel *lineModel() const;
        QAbstractItemModel *tableModel() const;

        int selectedTrack() const;
        void setSelectedTrack(int selectedTrack);

        QStringList filters() const;
        bool useSpaces() const;
        void setUseSpaces(bool useSpaces);

        int lyricsColumn() const;
        int tableRowCount() const;
        QString validationMessage() const;

        ExportState exportState() const;
        bool exported() const;
        void setDialogWindow(QWindow *window);

        Q_INVOKABLE void addFilter(const QString &filter);
        Q_INVOKABLE void removeFilter(const QString &filter);
        Q_INVOKABLE void insertBreak(int lineIndex, int wordIndex);
        Q_INVOKABLE void insertInterlude(int lineIndex);
        Q_INVOKABLE void removeBreak(int lineIndex);
        Q_INVOKABLE bool prepareTable();
        Q_INVOKABLE bool setTableCell(int row, int column, const QString &text);
        Q_INVOKABLE void exportFile();

    Q_SIGNALS:
        void selectedTrackChanged();
        void filtersChanged();
        void useSpacesChanged();
        void tableRowCountChanged();
        void validationMessageChanged();
        void exportStateChanged();

    private:
        std::unique_ptr<LyricExportSessionData> d;
    };

}

#endif // DIFFSCOPE_LYRIC_EXPORTER_LYRICEXPORTSESSION_H
