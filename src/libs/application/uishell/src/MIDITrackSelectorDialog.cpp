#include "MIDITrackSelectorDialog.h"
#include "MIDITrackSelectorDialog_p.h"

#include <uishell/MIDITrackSelectorStringConverter.h>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QSet>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>

namespace UIShell {

    class QtStringConverterStrategy : public MIDITrackSelectorStringConverter {
    public:
        QtStringConverterStrategy();
        ~QtStringConverterStrategy() override;

        QList<CodecInfo> availableCodecs() const override;
        QByteArray detectEncoding(const QByteArray &data) const override;
        QString decode(const QByteArray &data, const QByteArray &codec) const override;
        QByteArray defaultCodec() const override;
    };

    QtStringConverterStrategy::QtStringConverterStrategy() = default;

    QtStringConverterStrategy::~QtStringConverterStrategy() = default;

    QList<MIDITrackSelectorStringConverter::CodecInfo> QtStringConverterStrategy::availableCodecs() const {
        struct EncodingEntry {
            QStringConverter::Encoding encoding;
        };

        const QVector<EncodingEntry> orderedEncodings = {
            {QStringConverter::Utf8},
            {QStringConverter::System},
            {QStringConverter::Utf16},
            {QStringConverter::Utf16BE},
            {QStringConverter::Utf16LE},
            {QStringConverter::Utf32},
            {QStringConverter::Utf32BE},
            {QStringConverter::Utf32LE},
            {QStringConverter::Latin1},
        };

        QList<CodecInfo> result;
        QSet<QByteArray> seen;

        for (const auto &entry : orderedEncodings) {
            const QByteArray nameBytes = QStringConverter::nameForEncoding(entry.encoding);
            if (nameBytes.isEmpty())
                continue;
            const QByteArray normalized = nameBytes;
            if (seen.contains(normalized))
                continue;
            seen.insert(normalized);

            CodecInfo info;
            info.identifier = normalized;
            info.displayName = QString::fromLatin1(nameBytes);
            result.append(info);
        }

        return result;
    }

    QByteArray QtStringConverterStrategy::detectEncoding(const QByteArray &data) const {
        if (data.isEmpty())
            return {};

        const auto encoding = QStringConverter::encodingForData(data);
        if (encoding) {
            const QByteArray nameBytes = QStringConverter::nameForEncoding(*encoding);
            if (!nameBytes.isEmpty())
                return nameBytes;
        }

        return {};
    }

    QString QtStringConverterStrategy::decode(const QByteArray &data, const QByteArray &codec) const {
        const auto encoding = QStringConverter::encodingForName(codec);
        if (!encoding)
            return QString::fromUtf8(data);

        QStringDecoder decoder(*encoding);
        QString text = decoder.decode(data);
        if (decoder.hasError())
            text = QString::fromUtf8(data);

        return text;
    }

    QByteArray QtStringConverterStrategy::defaultCodec() const {
        return QStringConverter::nameForEncoding(QStringConverter::Utf8);
    }

    MIDITrackSelectorDialogPrivate::MIDITrackSelectorDialogPrivate(MIDITrackSelectorDialog *q, MIDITrackSelectorStringConverter *converter)
        : q_ptr(q), converter(converter ? converter : new QtStringConverterStrategy) {
        currentCodec = this->converter->defaultCodec();
    }

    MIDITrackSelectorDialogPrivate::~MIDITrackSelectorDialogPrivate() = default;

    void MIDITrackSelectorDialogPrivate::init() {
        Q_Q(MIDITrackSelectorDialog);

        auto *mainLayout = new QVBoxLayout(q);

        auto *codecLayout = new QHBoxLayout;
        auto *codecLabel = new QLabel(UIShell::MIDITrackSelectorDialog::tr("Encoding:"), q);
        codecComboBox = new QComboBox(q);
        codecLayout->addWidget(codecLabel);
        codecLayout->addWidget(codecComboBox, 1);
        mainLayout->addLayout(codecLayout);

        populateCodecCombo();
        syncComboToCodec();
        QObject::connect(codecComboBox, &QComboBox::currentIndexChanged, q, [this](int index) {
            if (index < 0)
                return;
            const QByteArray codecId = codecComboBox->itemData(index).toByteArray();
            Q_Q(MIDITrackSelectorDialog);
            q->setCodec(QString::fromLatin1(codecId));
        });

        auto *selectorGroup = new QGroupBox(UIShell::MIDITrackSelectorDialog::tr("Track Selector"), q);
        auto *selectorLayout = new QVBoxLayout(selectorGroup);

        auto *buttonLayout = new QHBoxLayout;
        selectAllCheckBox = new BinaryToggleCheckBox(UIShell::MIDITrackSelectorDialog::tr("Select All"), selectorGroup);
        selectAllCheckBox->setTristate(true);
        buttonLayout->addWidget(selectAllCheckBox);
        buttonLayout->addStretch();
        selectorLayout->addLayout(buttonLayout);

        trackView = new QTreeView(selectorGroup);
        trackView->setRootIsDecorated(false);
        trackView->setAlternatingRowColors(true);
        trackView->setSelectionBehavior(QAbstractItemView::SelectRows);
        trackView->setSelectionMode(QAbstractItemView::SingleSelection);
        selectorLayout->addWidget(trackView, 1);

        mainLayout->addWidget(selectorGroup, 2);

        auto *previewGroup = new QGroupBox(UIShell::MIDITrackSelectorDialog::tr("Preview"), q);
        auto *previewLayout = new QVBoxLayout(previewGroup);
        previewEdit = new QTextEdit(previewGroup);
        previewEdit->setReadOnly(true);
        previewEdit->setLineWrapMode(QTextEdit::WidgetWidth);
        previewLayout->addWidget(previewEdit);
        mainLayout->addWidget(previewGroup, 1);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, q);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, q, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, q, &QDialog::reject);
        mainLayout->addWidget(buttonBox);

        QObject::connect(selectAllCheckBox, &QCheckBox::checkStateChanged, q, [this](Qt::CheckState state) {
            if (!model)
                return;
            if (state == Qt::PartiallyChecked)
                return;
            QSignalBlocker blocker(model);
            const int rowCount = model->rowCount();
            for (int row = 0; row < rowCount; ++row) {
                auto *item = model->item(row, 0);
                if (!item)
                    continue;
                if (!(item->flags() & Qt::ItemIsEnabled))
                    continue;
                item->setCheckState(state);
            }
            if (trackView && trackView->viewport())
                trackView->viewport()->update();
            updateSelectedIndexes();
        });
        q->resize(640, 480);
    }

    void MIDITrackSelectorDialogPrivate::populateCodecCombo() {
        if (!codecComboBox || !converter)
            return;

        codecComboBox->clear();

        const auto codecs = converter->availableCodecs();
        for (const auto &codec : codecs) {
            QString itemText = codec.displayName;
            if (!autoDetectedCodec.isEmpty() && codec.identifier == autoDetectedCodec) {
                itemText = UIShell::MIDITrackSelectorDialog::tr("%1 (auto detected)").arg(codec.displayName);
            }
            codecComboBox->addItem(itemText, codec.identifier);
        }
    }

    void MIDITrackSelectorDialogPrivate::rebuildModel() {
        Q_Q(MIDITrackSelectorDialog);

        auto *newModel = new QStandardItemModel(q);
        newModel->setColumnCount(3);
        newModel->setHorizontalHeaderLabels({UIShell::MIDITrackSelectorDialog::tr("Name"), UIShell::MIDITrackSelectorDialog::tr("Range"), UIShell::MIDITrackSelectorDialog::tr("Note Count")});

        const int count = trackInfos.size();
        for (int i = 0; i < count; ++i) {
            const auto &info = trackInfos.at(i);

            auto *nameItem = new QStandardItem(decodedName(info.name));
            nameItem->setCheckable(true);
            Qt::ItemFlags nameFlags = Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
            if (info.disabled)
                nameFlags &= ~Qt::ItemIsEnabled;
            nameItem->setFlags(nameFlags);
            if (!info.disabled && info.selectedByDefault) {
                nameItem->setCheckState(Qt::Checked);
            } else {
                nameItem->setCheckState(Qt::Unchecked);
            }

            auto *rangeItem = new QStandardItem(info.rangeText);
            rangeItem->setFlags((rangeItem->flags() | Qt::ItemIsSelectable) & ~(Qt::ItemIsEditable | Qt::ItemIsUserCheckable));

            auto *noteCountItem = new QStandardItem(QString::number(info.noteCount));
            noteCountItem->setFlags((noteCountItem->flags() | Qt::ItemIsSelectable) & ~(Qt::ItemIsEditable | Qt::ItemIsUserCheckable));

            newModel->appendRow({nameItem, rangeItem, noteCountItem});
        }

        if (model) {
            model->deleteLater();
        }
        model = newModel;
        trackView->setModel(model);
        if (auto *header = trackView->header()) {
            header->setSectionResizeMode(0, QHeaderView::Stretch);
            header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
            header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        }

        QObject::connect(model, &QStandardItemModel::itemChanged, q, [this](QStandardItem *item) {
            if (!item)
                return;
            if (item->column() != 0)
                return;
            updateSelectedIndexes();
            updateSelectAllCheckBox();
        });

        QObject::connect(trackView->selectionModel(), &QItemSelectionModel::currentRowChanged, q,
                         [this](const QModelIndex &current, const QModelIndex &) {
                             updatePreviewForRow(current.row());
                         });

        if (model->rowCount() > 0) {
            trackView->setCurrentIndex(model->index(0, 0));
        }

        updateSelectedIndexes();
        updateSelectAllCheckBox();
        updatePreviewForRow(trackView->currentIndex().row());
    }

    void MIDITrackSelectorDialogPrivate::updateNamesForCodec() {
        if (!model)
            return;
        const int rowCount = model->rowCount();
        for (int row = 0; row < rowCount; ++row) {
            auto *item = model->item(row, 0);
            if (!item)
                continue;
            const auto &info = trackInfos.value(row);
            item->setText(decodedName(info.name));
        }
    }

    void MIDITrackSelectorDialogPrivate::updatePreviewForRow(int row) {
        if (!previewEdit)
            return;
        if (row < 0 || row >= trackInfos.size()) {
            previewEdit->clear();
            return;
        }
        const auto &info = trackInfos.at(row);
        previewEdit->setPlainText(decodeLyrics(info.lyrics));
    }

    void MIDITrackSelectorDialogPrivate::updateSelectedIndexes() {
        if (!model)
            return;
        QList<int> indexes;
        const int rowCount = model->rowCount();
        indexes.reserve(rowCount);
        for (int row = 0; row < rowCount; ++row) {
            const auto *item = model->item(row, 0);
            if (!item)
                continue;
            if (item->checkState() == Qt::Checked)
                indexes.append(row);
        }
        if (indexes == selectedIndexesCache)
            return;
        selectedIndexesCache = indexes;
        Q_Q(MIDITrackSelectorDialog);
        emit q->selectedIndexesChanged();
    }

    void MIDITrackSelectorDialogPrivate::updateSelectAllCheckBox() {
        if (!selectAllCheckBox || !model)
            return;
        int checkedCount = 0;
        int enabledCount = 0;
        const int rowCount = model->rowCount();
        for (int row = 0; row < rowCount; ++row) {
            const auto *item = model->item(row, 0);
            if (!item)
                continue;
            if (!(item->flags() & Qt::ItemIsEnabled))
                continue;
            enabledCount++;
            if (item->checkState() == Qt::Checked)
                checkedCount++;
        }
        QSignalBlocker blocker(selectAllCheckBox);
        if (enabledCount == 0) {
            selectAllCheckBox->setCheckState(Qt::Unchecked);
        } else if (checkedCount == 0) {
            selectAllCheckBox->setCheckState(Qt::Unchecked);
        } else if (checkedCount == enabledCount) {
            selectAllCheckBox->setCheckState(Qt::Checked);
        } else {
            selectAllCheckBox->setCheckState(Qt::PartiallyChecked);
        }
    }

    QString MIDITrackSelectorDialogPrivate::decodeBytes(const QByteArray &bytes) const {
        if (!converter)
            return QString::fromUtf8(bytes);
        return converter->decode(bytes, currentCodec);
    }

    QString MIDITrackSelectorDialogPrivate::decodedName(const QByteArray &bytes) const {
        return decodeBytes(bytes);
    }

    QString MIDITrackSelectorDialogPrivate::decodeLyrics(const QList<QByteArray> &lyrics) const {
        QStringList words;
        words.reserve(lyrics.size());
        for (const auto &word : lyrics) {
            words.append(decodeBytes(word));
        }
        return words.join(QLatin1Char(' '));
    }

    QByteArray MIDITrackSelectorDialogPrivate::aggregateLyricsForDetection() const {
        QByteArray combined;
        for (const auto &info : trackInfos) {
            QByteArray lyricsJoined;
            for (int i = 0; i < info.lyrics.size(); ++i) {
                if (i > 0)
                    lyricsJoined.append(' ');
                lyricsJoined.append(info.lyrics.at(i));
            }
            QByteArray trackData = info.name;
            trackData.append(lyricsJoined);
            combined.append(trackData);
        }
        return combined;
    }

    void MIDITrackSelectorDialogPrivate::syncComboToCodec() {
        if (!codecComboBox)
            return;
        const QByteArray normalized = currentCodec;
        const int count = codecComboBox->count();
        for (int i = 0; i < count; ++i) {
            if (codecComboBox->itemData(i).toByteArray() == normalized) {
                codecComboBox->setCurrentIndex(i);
                return;
            }
        }
    }

    MIDITrackSelectorDialog::MIDITrackSelectorDialog(QWidget *parent, MIDITrackSelectorStringConverter *converter)
        : QDialog(parent), d_ptr(new MIDITrackSelectorDialogPrivate(this, converter)) {
        Q_D(MIDITrackSelectorDialog);
        d->init();
    }

    MIDITrackSelectorDialog::~MIDITrackSelectorDialog() = default;

    QList<MIDITrackSelectorDialog::TrackInfo> MIDITrackSelectorDialog::trackInfoList() const {
        Q_D(const MIDITrackSelectorDialog);
        return d->trackInfos;
    }

    void MIDITrackSelectorDialog::setTrackInfoList(const QList<TrackInfo> &trackInfoList) {
        Q_D(MIDITrackSelectorDialog);
        d->trackInfos = trackInfoList;
        d->autoDetectedCodec.clear();
        d->populateCodecCombo();
        d->rebuildModel();
    }

    QString MIDITrackSelectorDialog::codec() const {
        Q_D(const MIDITrackSelectorDialog);
        return QString::fromLatin1(d->currentCodec);
    }

    void MIDITrackSelectorDialog::setCodec(const QString &codec) {
        Q_D(MIDITrackSelectorDialog);
        const QByteArray normalized = codec.toLatin1();
        if (normalized.isEmpty())
            return;
        if (normalized == d->currentCodec)
            return;
        d->currentCodec = normalized;
        d->syncComboToCodec();
        d->updateNamesForCodec();
        d->updatePreviewForRow(d->trackView ? d->trackView->currentIndex().row() : -1);
        emit codecChanged(codec);
    }

    QList<int> MIDITrackSelectorDialog::selectedIndexes() const {
        Q_D(const MIDITrackSelectorDialog);
        return d->selectedIndexesCache;
    }

    void MIDITrackSelectorDialog::detectCodec() {
        Q_D(MIDITrackSelectorDialog);
        if (!d->converter)
            return;
        const QByteArray combined = d->aggregateLyricsForDetection();
        if (combined.isEmpty())
            return;
        const QByteArray detected = d->converter->detectEncoding(combined);
        if (!detected.isEmpty()) {
            d->autoDetectedCodec = detected;
            d->populateCodecCombo();
            setCodec(QString::fromLatin1(detected));
        }
    }

}

#include "MIDITrackSelectorDialog.moc"
