// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "LyricExportSession.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>

#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QByteArray>
#include <QDir>
#include <QHash>
#include <QIODevice>
#include <QList>
#include <QLoggingCategory>
#include <QPair>
#include <QPointer>
#include <QSaveFile>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>
#include <QWindow>
#include <QtGlobal>

#include <CoreApi/runtimeinterface.h>

#include <opendspx/model.h>
#include <opendspx/singingclip.h>

#include <SVSCraftCore/LongTime.h>
#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftQuick/MessageBox.h>

#include <lyricexporter/internal/LyricExportFormat.h>

namespace LyricExporter::Internal {

    constexpr int kLrcInterludeGapThresholdTicks = 1000;
    constexpr int kTicksPerQuarterNote = 480;

    Q_STATIC_LOGGING_CATEGORY(lcLyricExportSession, "diffscope.lyricexporter.session")

    struct LyricWordData {
        QString lyric;
        int startTick{};
        int endTick{};
        int startMsec{};
        int endMsec{};
    };

    struct LyricTrackData {
        QString name;
        QList<LyricWordData> words;
        QString warningText;
        bool selectable{};
    };

    struct LyricLineData {
        QList<LyricWordData> words;
        bool interlude{};
    };

    class LyricTrackModel;
    class LyricLineModel;
    class LyricTableModel;

    class LyricExportSessionData {
    public:
        LyricExportSessionData(LyricExportSession *q, const LyricExportFormat &format,
                               QString path, const opendspx::Model &model, QWindow *window);
        ~LyricExportSessionData();

        void buildTracks(const opendspx::Model &model);
        void rebuildLines();
        void clearTable();
        void markStructureChanged();
        void setValidationMessage(const QString &message);
        void setExportState(LyricExportSession::ExportState state);
        void reportFileError(const QString &detail);

        LyricExportSession *q;
        const LyricExportFormat *format;
        QString path;
        QPointer<QWindow> window;
        QList<LyricTrackData> tracks;
        QList<LyricLineData> lines;
        QList<LyricExportRow> tableRows;
        QStringList filters{QStringLiteral("+"), QStringLiteral("-")};
        QSet<QString> filterSet{QStringLiteral("+"), QStringLiteral("-")};
        bool useSpaces{true};
        int selectedTrack{-1};
        quint64 editRevision{};
        quint64 preparedRevision{};
        bool tablePrepared{};
        QString validationMessage;
        LyricExportSession::ExportState exportState{LyricExportSession::Idle};
        LyricTrackModel *trackModel{};
        LyricLineModel *lineModel{};
        LyricTableModel *tableModel{};
    };

    class LyricTrackModel : public QAbstractListModel {
    public:
        enum Role {
            DisplayTextRole = Qt::UserRole + 1,
            SelectableRole,
            WarningTextRole,
        };

        explicit LyricTrackModel(LyricExportSessionData *data, QObject *parent)
            : QAbstractListModel(parent), m_data(data) {
        }

        int rowCount(const QModelIndex &parent = {}) const override {
            return parent.isValid() ? 0 : m_data->tracks.size();
        }

        QVariant data(const QModelIndex &index, int role) const override {
            if (!index.isValid() || index.row() < 0 || index.row() >= m_data->tracks.size())
                return {};
            const auto &track = m_data->tracks.at(index.row());
            switch (role) {
                case DisplayTextRole:
                    return LyricExportSession::tr("%L1. %2")
                        .arg(index.row() + 1)
                        .arg(track.name);
                case SelectableRole:
                    return track.selectable;
                case WarningTextRole:
                    return track.warningText;
                default:
                    return {};
            }
        }

        QHash<int, QByteArray> roleNames() const override {
            return {
                {DisplayTextRole, QByteArrayLiteral("displayText")},
                {SelectableRole, QByteArrayLiteral("selectable")},
                {WarningTextRole, QByteArrayLiteral("warningText")},
            };
        }

    private:
        LyricExportSessionData *m_data;
    };

    class LyricLineModel : public QAbstractListModel {
    public:
        enum Role {
            WordsRole = Qt::UserRole + 1,
            InterludeRole,
            CanInsertInterludeRole,
            CanRemoveBreakRole,
        };

        explicit LyricLineModel(LyricExportSessionData *data, QObject *parent)
            : QAbstractListModel(parent), m_data(data) {
        }

        int rowCount(const QModelIndex &parent = {}) const override {
            return parent.isValid() ? 0 : m_data->lines.size();
        }

        QVariant data(const QModelIndex &index, int role) const override {
            if (!index.isValid() || index.row() < 0 || index.row() >= m_data->lines.size())
                return {};
            const auto &line = m_data->lines.at(index.row());
            switch (role) {
                case WordsRole: {
                    QVariantList words;
                    words.reserve(line.words.size());
                    for (const auto &word : line.words) {
                        words.append(QVariantMap{
                            {QStringLiteral("text"), word.lyric},
                            {QStringLiteral("filtered"), m_data->filterSet.contains(word.lyric)},
                        });
                    }
                    return words;
                }
                case InterludeRole:
                    return line.interlude;
                case CanInsertInterludeRole:
                    return m_data->format->supportsInterludes && !line.interlude
                        && (index.row() + 1 >= m_data->lines.size()
                            || !m_data->lines.at(index.row() + 1).interlude);
                case CanRemoveBreakRole:
                    return index.row() + 1 < m_data->lines.size();
                default:
                    return {};
            }
        }

        QHash<int, QByteArray> roleNames() const override {
            return {
                {WordsRole, QByteArrayLiteral("words")},
                {InterludeRole, QByteArrayLiteral("interlude")},
                {CanInsertInterludeRole, QByteArrayLiteral("canInsertInterlude")},
                {CanRemoveBreakRole, QByteArrayLiteral("canRemoveBreak")},
            };
        }

        void beginSessionReset() {
            beginResetModel();
        }

        void endSessionReset() {
            endResetModel();
        }

        void beginSessionInsert(int row) {
            beginInsertRows({}, row, row);
        }

        void endSessionInsert() {
            endInsertRows();
        }

        void beginSessionRemove(int row) {
            beginRemoveRows({}, row, row);
        }

        void endSessionRemove() {
            endRemoveRows();
        }

        void refreshWords() {
            if (!m_data->lines.isEmpty())
                Q_EMIT dataChanged(index(0), index(m_data->lines.size() - 1), {WordsRole});
        }

        void refreshRows(int first, int last, const QList<int> &roles = {}) {
            if (m_data->lines.isEmpty())
                return;
            const int lastRow = static_cast<int>(m_data->lines.size()) - 1;
            first = std::clamp(first, 0, lastRow);
            last = std::clamp(last, first, lastRow);
            Q_EMIT dataChanged(index(first), index(last), roles);
        }

    private:
        LyricExportSessionData *m_data;
    };

    class LyricTableModel : public QAbstractTableModel {
    public:
        explicit LyricTableModel(LyricExportSessionData *data, QObject *parent)
            : QAbstractTableModel(parent), m_data(data) {
        }

        int rowCount(const QModelIndex &parent = {}) const override {
            return parent.isValid() ? 0 : m_data->tableRows.size();
        }

        int columnCount(const QModelIndex &parent = {}) const override {
            return parent.isValid() ? 0 : m_data->format->columnCount();
        }

        QVariant data(const QModelIndex &index, int role) const override {
            if (!index.isValid() || index.row() < 0
                || index.row() >= m_data->tableRows.size()) {
                return {};
            }
            const auto &row = m_data->tableRows.at(index.row());
            if (role != Qt::DisplayRole && role != Qt::EditRole)
                return {};
            const int lyricColumn = m_data->format->lyricColumn();
            if (index.column() == lyricColumn)
                return row.lyric;
            if (index.column() == 0)
                return SVS::LongTime(row.startMsec).toString();
            if (index.column() == 1)
                return SVS::LongTime(row.endMsec).toString();
            return {};
        }

        bool setData(const QModelIndex &index, const QVariant &value,
                     int role = Qt::EditRole) override {
            if (role != Qt::EditRole || !index.isValid())
                return false;
            return m_data->q->setTableCell(index.row(), index.column(), value.toString());
        }

        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const override {
            if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
                return {};
            const int lyricColumn = m_data->format->lyricColumn();
            if (section == lyricColumn)
                return LyricExportSession::tr("Lyrics");
            if (section == 0)
                return LyricExportSession::tr("Start");
            if (section == 1)
                return LyricExportSession::tr("End");
            return {};
        }

        Qt::ItemFlags flags(const QModelIndex &index) const override {
            return index.isValid()
                ? QAbstractTableModel::flags(index) | Qt::ItemIsEditable
                : Qt::NoItemFlags;
        }

        QHash<int, QByteArray> roleNames() const override {
            return {
                {Qt::DisplayRole, QByteArrayLiteral("display")},
            };
        }

        void beginSessionReset() {
            beginResetModel();
        }

        void endSessionReset() {
            endResetModel();
        }

        void refreshCell(int row, int column) {
            const auto modelIndex = index(row, column);
            Q_EMIT dataChanged(modelIndex, modelIndex, {Qt::DisplayRole, Qt::EditRole});
        }

    private:
        LyricExportSessionData *m_data;
    };

    static std::optional<int> tickToMsec(const SVS::MusicTimeline &timeline, int tick) {
        const double value = timeline.create(0, 0, tick).millisecond();
        if (!std::isfinite(value))
            return std::nullopt;
        const qint64 rounded = qRound64(value);
        if (rounded < 0 || rounded > std::numeric_limits<int>::max())
            return std::nullopt;
        return static_cast<int>(rounded);
    }

    LyricExportSessionData::LyricExportSessionData(
        LyricExportSession *q, const LyricExportFormat &format, QString path,
        const opendspx::Model &model, QWindow *window
    ) : q(q), format(&format), path(std::move(path)), window(window) {
        trackModel = new LyricTrackModel(this, q);
        lineModel = new LyricLineModel(this, q);
        tableModel = new LyricTableModel(this, q);
        buildTracks(model);
        for (int i = 0; i < tracks.size(); ++i) {
            if (tracks.at(i).selectable) {
                selectedTrack = i;
                break;
            }
        }
        rebuildLines();
    }

    LyricExportSessionData::~LyricExportSessionData() {
        delete trackModel;
        delete lineModel;
        delete tableModel;
    }

    void LyricExportSessionData::buildTracks(const opendspx::Model &model) {
        SVS::MusicTimeline timeline(kTicksPerQuarterNote);
        QList<QPair<int, double>> tempos;
        tempos.reserve(static_cast<qsizetype>(model.content.timeline.tempos.size()));
        for (const auto &tempo : model.content.timeline.tempos)
            tempos.append({tempo.pos, tempo.value});
        timeline.setTempi(tempos);

        tracks.reserve(static_cast<qsizetype>(model.content.tracks.size()));
        for (const auto &sourceTrack : model.content.tracks) {
            LyricTrackData track;
            track.name = QString::fromStdString(sourceTrack.name);
            qsizetype wordCapacity = 0;
            for (const auto &clip : sourceTrack.clips) {
                if (clip && clip->type == opendspx::Clip::Type::Singing) {
                    const auto singingClip = std::static_pointer_cast<opendspx::SingingClip>(clip);
                    wordCapacity += static_cast<qsizetype>(singingClip->notes.size());
                }
            }
            track.words.reserve(wordCapacity);
            bool containsVisibleNote = false;
            bool timeRangeValid = true;

            for (const auto &clip : sourceTrack.clips) {
                if (!clip || clip->type != opendspx::Clip::Type::Singing
                    || clip->time.clipLen <= 0) {
                    continue;
                }
                const auto singingClip = std::static_pointer_cast<opendspx::SingingClip>(clip);
                const qint64 sourceClipStart = clip->time.clipStart;
                const qint64 sourceClipEnd = sourceClipStart + clip->time.clipLen;

                for (const auto &note : singingClip->notes) {
                    const qint64 noteStart = note.pos;
                    const qint64 noteEnd = noteStart + std::max(0, note.length);
                    const bool pointNoteVisible = note.length == 0
                        && noteStart >= sourceClipStart && noteStart < sourceClipEnd;
                    const bool rangedNoteVisible = note.length > 0
                        && noteEnd > sourceClipStart && noteStart < sourceClipEnd;
                    if (!pointNoteVisible && !rangedNoteVisible)
                        continue;

                    containsVisibleNote = true;
                    const qint64 visibleStart = pointNoteVisible
                        ? noteStart
                        : std::max(noteStart, sourceClipStart);
                    const qint64 visibleEnd = pointNoteVisible
                        ? noteStart
                        : std::min(noteEnd, sourceClipEnd);
                    const qint64 projectStart = clip->time.pos + visibleStart - sourceClipStart;
                    const qint64 projectEnd = clip->time.pos + visibleEnd - sourceClipStart;
                    if (projectStart < 0 || projectEnd < 0
                        || projectStart > std::numeric_limits<int>::max()
                        || projectEnd > std::numeric_limits<int>::max()) {
                        timeRangeValid = false;
                        continue;
                    }
                    const auto startMsec = tickToMsec(timeline, static_cast<int>(projectStart));
                    const auto endMsec = tickToMsec(timeline, static_cast<int>(projectEnd));
                    if (!startMsec || !endMsec) {
                        timeRangeValid = false;
                        continue;
                    }
                    track.words.append({
                        QString::fromStdString(note.lyric),
                        static_cast<int>(projectStart),
                        static_cast<int>(projectEnd),
                        *startMsec,
                        *endMsec,
                    });
                }
            }

            std::stable_sort(track.words.begin(), track.words.end(), [](const auto &left, const auto &right) {
                return left.startTick < right.startTick;
            });

            bool overlapped = false;
            int furthestEnd = 0;
            for (const auto &word : std::as_const(track.words)) {
                if (word.startTick < furthestEnd) {
                    overlapped = true;
                    break;
                }
                furthestEnd = std::max(furthestEnd, word.endTick);
            }

            if (!containsVisibleNote) {
                track.warningText = LyricExportSession::tr("This track contains no notes.");
            } else if (!timeRangeValid) {
                track.warningText = LyricExportSession::tr("This track exceeds the supported time range.");
            } else if (overlapped) {
                track.warningText = LyricExportSession::tr("This track contains overlapping notes.");
            }
            track.selectable = containsVisibleNote && timeRangeValid && !overlapped;
            tracks.append(std::move(track));
        }
    }

    void LyricExportSessionData::rebuildLines() {
        lines.clear();
        if (selectedTrack < 0 || selectedTrack >= tracks.size()
            || !tracks.at(selectedTrack).selectable) {
            return;
        }
        const auto &words = tracks.at(selectedTrack).words;
        if (words.isEmpty())
            return;

        LyricLineData currentLine;
        currentLine.words.append(words.constFirst());
        for (qsizetype i = 1; i < words.size(); ++i) {
            const auto &previous = words.at(i - 1);
            const auto &word = words.at(i);
            const int gap = word.startTick - previous.endTick;
            if (gap > 0) {
                lines.append(std::move(currentLine));
                currentLine = {};
                if (format->supportsInterludes
                    && gap > kLrcInterludeGapThresholdTicks) {
                    lines.append({{}, true});
                }
            }
            currentLine.words.append(word);
        }
        lines.append(std::move(currentLine));
        if (format->supportsInterludes)
            lines.append({{}, true});
    }

    void LyricExportSessionData::clearTable() {
        const bool rowCountChanged = !tableRows.isEmpty();
        tableModel->beginSessionReset();
        tableRows.clear();
        tableModel->endSessionReset();
        tablePrepared = false;
        if (rowCountChanged)
            Q_EMIT q->tableRowCountChanged();
    }

    void LyricExportSessionData::markStructureChanged() {
        ++editRevision;
        tablePrepared = false;
        setValidationMessage({});
    }

    void LyricExportSessionData::setValidationMessage(const QString &message) {
        if (validationMessage == message)
            return;
        validationMessage = message;
        Q_EMIT q->validationMessageChanged();
    }

    void LyricExportSessionData::setExportState(LyricExportSession::ExportState state) {
        if (exportState == state)
            return;
        exportState = state;
        Q_EMIT q->exportStateChanged();
    }

    void LyricExportSessionData::reportFileError(const QString &detail) {
        const QString nativePath = QDir::toNativeSeparators(path);
        setExportState(LyricExportSession::Failed);
        SVS::MessageBox::critical(
            Core::RuntimeInterface::qmlEngine(), window,
            LyricExportSession::tr("Failed to Save File"),
            QStringLiteral("%1\n\n%2").arg(nativePath, detail)
        );
    }

    LyricExportSession::LyricExportSession(
        const LyricExportFormat &format, const QString &path,
        const opendspx::Model &model, QWindow *window,
        QObject *parent
    ) : QObject(parent), d(std::make_unique<LyricExportSessionData>(this, format, path, model, window)) {
    }

    LyricExportSession::~LyricExportSession() = default;

    QAbstractItemModel *LyricExportSession::trackModel() const {
        return d->trackModel;
    }

    QAbstractItemModel *LyricExportSession::lineModel() const {
        return d->lineModel;
    }

    QAbstractItemModel *LyricExportSession::tableModel() const {
        return d->tableModel;
    }

    int LyricExportSession::selectedTrack() const {
        return d->selectedTrack;
    }

    void LyricExportSession::setSelectedTrack(int selectedTrack) {
        if (selectedTrack == d->selectedTrack || selectedTrack < 0
            || selectedTrack >= d->tracks.size() || !d->tracks.at(selectedTrack).selectable) {
            return;
        }
        d->lineModel->beginSessionReset();
        d->selectedTrack = selectedTrack;
        d->rebuildLines();
        d->lineModel->endSessionReset();
        d->markStructureChanged();
        d->clearTable();
        Q_EMIT selectedTrackChanged();
    }

    QStringList LyricExportSession::filters() const {
        return d->filters;
    }

    bool LyricExportSession::useSpaces() const {
        return d->useSpaces;
    }

    void LyricExportSession::setUseSpaces(bool useSpaces) {
        if (d->useSpaces == useSpaces)
            return;
        d->useSpaces = useSpaces;
        d->markStructureChanged();
        Q_EMIT useSpacesChanged();
    }

    int LyricExportSession::lyricsColumn() const {
        return d->format->lyricColumn();
    }

    int LyricExportSession::tableRowCount() const {
        return d->tableRows.size();
    }

    QString LyricExportSession::validationMessage() const {
        return d->validationMessage;
    }

    LyricExportSession::ExportState LyricExportSession::exportState() const {
        return d->exportState;
    }

    bool LyricExportSession::exported() const {
        return d->exportState == Succeeded;
    }

    void LyricExportSession::setDialogWindow(QWindow *window) {
        d->window = window;
    }

    void LyricExportSession::addFilter(const QString &filter) {
        const QString normalized = filter.trimmed();
        if (normalized.isEmpty() || d->filterSet.contains(normalized))
            return;
        d->filters.append(normalized);
        d->filterSet.insert(normalized);
        d->markStructureChanged();
        d->lineModel->refreshWords();
        Q_EMIT filtersChanged();
    }

    void LyricExportSession::removeFilter(const QString &filter) {
        const qsizetype index = d->filters.indexOf(filter);
        if (index < 0)
            return;
        d->filters.removeAt(index);
        d->filterSet.remove(filter);
        d->markStructureChanged();
        d->lineModel->refreshWords();
        Q_EMIT filtersChanged();
    }

    void LyricExportSession::insertBreak(int lineIndex, int wordIndex) {
        if (lineIndex < 0 || lineIndex >= d->lines.size())
            return;
        auto &line = d->lines[lineIndex];
        if (line.interlude || wordIndex < 0 || wordIndex + 1 >= line.words.size())
            return;

        LyricLineData nextLine;
        nextLine.words = line.words.mid(wordIndex + 1);
        d->lineModel->beginSessionInsert(lineIndex + 1);
        line.words.resize(wordIndex + 1);
        d->lines.insert(lineIndex + 1, std::move(nextLine));
        d->lineModel->endSessionInsert();
        d->lineModel->refreshRows(lineIndex, lineIndex + 1);
        d->markStructureChanged();
    }

    void LyricExportSession::insertInterlude(int lineIndex) {
        if (!d->format->supportsInterludes || lineIndex < 0 || lineIndex >= d->lines.size()
            || d->lines.at(lineIndex).interlude
            || (lineIndex + 1 < d->lines.size() && d->lines.at(lineIndex + 1).interlude)) {
            return;
        }
        d->lineModel->beginSessionInsert(lineIndex + 1);
        d->lines.insert(lineIndex + 1, LyricLineData{{}, true});
        d->lineModel->endSessionInsert();
        d->lineModel->refreshRows(lineIndex, lineIndex + 1);
        d->markStructureChanged();
    }

    void LyricExportSession::removeBreak(int lineIndex) {
        if (lineIndex < 0 || lineIndex + 1 >= d->lines.size())
            return;
        auto &current = d->lines[lineIndex];
        auto &next = d->lines[lineIndex + 1];
        d->lineModel->beginSessionRemove(lineIndex + 1);
        if (current.interlude) {
            current = std::move(next);
        } else if (!next.interlude) {
            current.words.append(std::move(next.words));
        }
        d->lines.removeAt(lineIndex + 1);
        d->lineModel->endSessionRemove();
        d->lineModel->refreshRows(std::max(0, lineIndex - 1), lineIndex + 1);
        d->markStructureChanged();
    }

    bool LyricExportSession::prepareTable() {
        if (d->selectedTrack < 0 || d->selectedTrack >= d->tracks.size()
            || !d->tracks.at(d->selectedTrack).selectable) {
            d->setValidationMessage(tr("Select an available track before continuing."));
            return false;
        }
        if (d->tablePrepared && d->preparedRevision == d->editRevision)
            return true;

        QList<LyricExportRow> rows;
        rows.reserve(d->lines.size());
        std::optional<int> precedingLineEnd;
        for (const auto &line : std::as_const(d->lines)) {
            if (line.interlude) {
                const int start = precedingLineEnd.value_or(0);
                rows.append({start, start, {}});
                continue;
            }
            if (line.words.isEmpty())
                continue;

            precedingLineEnd = line.words.constLast().endMsec;
            QStringList lyrics;
            lyrics.reserve(line.words.size());
            for (const auto &word : line.words) {
                if (!word.lyric.isEmpty() && !d->filterSet.contains(word.lyric))
                    lyrics.append(word.lyric);
            }
            if (lyrics.isEmpty())
                continue;
            rows.append({
                line.words.constFirst().startMsec,
                line.words.constLast().endMsec,
                lyrics.join(d->useSpaces ? QStringLiteral(" ") : QString()),
            });
        }

        const int oldRowCount = d->tableRows.size();
        d->tableModel->beginSessionReset();
        d->tableRows = std::move(rows);
        d->tableModel->endSessionReset();
        d->preparedRevision = d->editRevision;
        d->tablePrepared = true;
        d->setValidationMessage({});
        if (oldRowCount != d->tableRows.size())
            Q_EMIT tableRowCountChanged();
        return true;
    }

    bool LyricExportSession::setTableCell(int row, int column, const QString &text) {
        if (row < 0 || row >= d->tableRows.size() || column < 0
            || column >= d->tableModel->columnCount()) {
            return false;
        }
        auto &tableRow = d->tableRows[row];
        if (column == lyricsColumn()) {
            if (tableRow.lyric == text) {
                d->setValidationMessage({});
                return true;
            }
            tableRow.lyric = text;
            d->setValidationMessage({});
            d->tableModel->refreshCell(row, column);
            return true;
        }

        bool ok = false;
        const auto time = SVS::LongTime::fromString(text, &ok);
        if (!ok || time.negative()) {
            d->setValidationMessage(tr("Enter a valid non-negative time code."));
            return false;
        }
        const int msec = time.totalMillisecond();
        if (column == 0) {
            if (d->format->type == LyricExportFormatType::StartEndTime
                && msec > tableRow.endMsec) {
                d->setValidationMessage(tr("The start time cannot be later than the end time."));
                return false;
            }
            tableRow.startMsec = msec;
        } else if (column == 1
                   && d->format->type == LyricExportFormatType::StartEndTime) {
            if (msec < tableRow.startMsec) {
                d->setValidationMessage(tr("The end time cannot be earlier than the start time."));
                return false;
            }
            tableRow.endMsec = msec;
        } else {
            return false;
        }
        d->setValidationMessage({});
        d->tableModel->refreshCell(row, column);
        return true;
    }

    void LyricExportSession::exportFile() {
        if (d->exportState == Exporting)
            return;
        if (!prepareTable()) {
            d->setExportState(Failed);
            return;
        }
        d->setExportState(Exporting);

        QTimer::singleShot(0, this, [this] {
            const QByteArray data = d->format->serialize(d->tableRows);
            QSaveFile file(d->path);
            if (!file.open(QIODevice::WriteOnly)) {
                qCCritical(lcLyricExportSession) << "Failed to open output file:" << d->path
                                                 << file.errorString();
                d->reportFileError(file.errorString());
                return;
            }
            if (file.write(data) != data.size()) {
                const QString detail = file.errorString().isEmpty()
                    ? tr("Could not write all data to the file.")
                    : file.errorString();
                qCCritical(lcLyricExportSession) << "Failed to write output file:" << d->path
                                                 << detail;
                file.cancelWriting();
                d->reportFileError(detail);
                return;
            }
            if (!file.commit()) {
                qCCritical(lcLyricExportSession) << "Failed to commit output file:" << d->path
                                                 << file.errorString();
                d->reportFileError(file.errorString());
                return;
            }
            d->setExportState(Succeeded);
        });
    }

}
