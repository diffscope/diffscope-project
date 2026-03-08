#include "MIDITrackSelectorDialog_p.h"
#include "MIDITrackSelectorDialog.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSet>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>

#include <midiformatconverter/internal/MIDITextCodecConverter.h>

namespace MIDIFormatConverter::Internal {

    MIDITrackSelectorDialogPrivate::MIDITrackSelectorDialogPrivate(MIDITrackSelectorDialog *q) : q_ptr(q) {
        currentCodec = MIDITextCodecConverter::defaultCodec();
    }

    MIDITrackSelectorDialogPrivate::~MIDITrackSelectorDialogPrivate() = default;

    void MIDITrackSelectorDialogPrivate::init() {
        Q_Q(MIDITrackSelectorDialog);

        auto *mainLayout = new QVBoxLayout(q);

        auto *codecLayout = new QHBoxLayout;
        auto *codecLabel = new QLabel(MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Encoding:"), q);
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
            q->setCodec(codecId);
        });

        auto *selectorGroup = new QGroupBox(MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Track Selector"), q);
        auto *selectorLayout = new QVBoxLayout(selectorGroup);

        auto *buttonLayout = new QHBoxLayout;
        selectAllCheckBox = new BinaryToggleCheckBox(MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Select All"), selectorGroup);
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

        auto *previewGroup = new QGroupBox(MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Preview"), q);
        auto *previewLayout = new QVBoxLayout(previewGroup);
        previewEdit = new QTextEdit(previewGroup);
        previewEdit->setReadOnly(true);
        previewEdit->setLineWrapMode(QTextEdit::WidgetWidth);
        previewLayout->addWidget(previewEdit);
        mainLayout->addWidget(previewGroup, 1);

        auto *optionsGroup = new QGroupBox(MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Options"), q);
        auto *optionsLayout = new QVBoxLayout(optionsGroup);

        separateMidiChannelsCheckBox = new QCheckBox(MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Separate MIDI channels"), optionsGroup);
        separateMidiChannelsCheckBox->setChecked(separateMidiChannels);
        optionsLayout->addWidget(separateMidiChannelsCheckBox);

        importTempoCheckBox = new QCheckBox(MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Import tempo"), optionsGroup);
        importTempoCheckBox->setChecked(importTempo);
        optionsLayout->addWidget(importTempoCheckBox);

        importTimeSignatureCheckBox = new QCheckBox(MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Import time signature"), optionsGroup);
        importTimeSignatureCheckBox->setChecked(importTimeSignature);
        optionsLayout->addWidget(importTimeSignatureCheckBox);

        mainLayout->addWidget(optionsGroup);

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

        QObject::connect(separateMidiChannelsCheckBox, &QCheckBox::toggled, q, [this](bool checked) {
            Q_Q(MIDITrackSelectorDialog);
            q->setseparateMidiChannels(checked);
        });

        QObject::connect(importTempoCheckBox, &QCheckBox::toggled, q, [this](bool checked) {
            Q_Q(MIDITrackSelectorDialog);
            q->setImportTempo(checked);
        });

        QObject::connect(importTimeSignatureCheckBox, &QCheckBox::toggled, q, [this](bool checked) {
            Q_Q(MIDITrackSelectorDialog);
            q->setImportTimeSignature(checked);
        });
        q->resize(640, 480);
    }

    void MIDITrackSelectorDialogPrivate::populateCodecCombo() {
        if (!codecComboBox)
            return;

        codecComboBox->clear();

        const auto codecs = MIDITextCodecConverter::availableCodecs();
        for (const auto &codec : codecs) {
            QString itemText = codec.displayName;
            if (!autoDetectedCodec.isEmpty() && codec.identifier == autoDetectedCodec) {
                itemText = MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("%1 (auto detected)").arg(codec.displayName);
            }
            codecComboBox->addItem(itemText, codec.identifier);
        }
    }

    void MIDITrackSelectorDialogPrivate::rebuildModel() {
        Q_Q(MIDITrackSelectorDialog);

        auto *newModel = new QStandardItemModel(q);
        newModel->setColumnCount(3);
        newModel->setHorizontalHeaderLabels({MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Name"), MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Range"), MIDIFormatConverter::Internal::MIDITrackSelectorDialog::tr("Note Count")});

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
        return MIDITextCodecConverter::decode(bytes, currentCodec);
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

    MIDITrackSelectorDialog::MIDITrackSelectorDialog(QWidget *parent)
        : QDialog(parent), d_ptr(new MIDITrackSelectorDialogPrivate(this)) {
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

    QByteArray MIDITrackSelectorDialog::codec() const {
        Q_D(const MIDITrackSelectorDialog);
        return d->currentCodec;
    }

    void MIDITrackSelectorDialog::setCodec(const QByteArray &codec) {
        Q_D(MIDITrackSelectorDialog);
        if (codec.isEmpty())
            return;
        if (codec == d->currentCodec)
            return;
        d->currentCodec = codec;
        d->syncComboToCodec();
        d->updateNamesForCodec();
        d->updatePreviewForRow(d->trackView ? d->trackView->currentIndex().row() : -1);
        emit codecChanged(codec);
    }

    QList<int> MIDITrackSelectorDialog::selectedIndexes() const {
        Q_D(const MIDITrackSelectorDialog);
        return d->selectedIndexesCache;
    }

    bool MIDITrackSelectorDialog::separateMidiChannels() const {
        Q_D(const MIDITrackSelectorDialog);
        return d->separateMidiChannels;
    }

    void MIDITrackSelectorDialog::setseparateMidiChannels(bool enabled) {
        Q_D(MIDITrackSelectorDialog);
        if (d->separateMidiChannels == enabled)
            return;
        d->separateMidiChannels = enabled;
        if (d->separateMidiChannelsCheckBox) {
            QSignalBlocker blocker(d->separateMidiChannelsCheckBox);
            d->separateMidiChannelsCheckBox->setChecked(enabled);
        }
        emit separateMidiChannelsChanged(enabled);
    }

    bool MIDITrackSelectorDialog::importTempo() const {
        Q_D(const MIDITrackSelectorDialog);
        return d->importTempo;
    }

    void MIDITrackSelectorDialog::setImportTempo(bool enabled) {
        Q_D(MIDITrackSelectorDialog);
        if (d->importTempo == enabled)
            return;
        d->importTempo = enabled;
        if (d->importTempoCheckBox) {
            QSignalBlocker blocker(d->importTempoCheckBox);
            d->importTempoCheckBox->setChecked(enabled);
        }
        emit importTempoChanged(enabled);
    }

    bool MIDITrackSelectorDialog::importTimeSignature() const {
        Q_D(const MIDITrackSelectorDialog);
        return d->importTimeSignature;
    }

    void MIDITrackSelectorDialog::setImportTimeSignature(bool enabled) {
        Q_D(MIDITrackSelectorDialog);
        if (d->importTimeSignature == enabled)
            return;
        d->importTimeSignature = enabled;
        if (d->importTimeSignatureCheckBox) {
            QSignalBlocker blocker(d->importTimeSignatureCheckBox);
            d->importTimeSignatureCheckBox->setChecked(enabled);
        }
        emit importTimeSignatureChanged(enabled);
    }

    void MIDITrackSelectorDialog::detectCodec() {
        Q_D(MIDITrackSelectorDialog);
        const QByteArray combined = d->aggregateLyricsForDetection();
        if (combined.isEmpty())
            return;
        const QByteArray detected = MIDITextCodecConverter::detectEncoding(combined);
        if (!detected.isEmpty()) {
            d->autoDetectedCodec = detected;
            d->populateCodecCombo();
            setCodec(detected);
        }
    }

}

#include "MIDITrackSelectorDialog.moc"
