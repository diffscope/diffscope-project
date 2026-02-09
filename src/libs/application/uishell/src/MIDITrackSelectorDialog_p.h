#ifndef DIFFSCOPE_UISHELL_MIDITRACKSELECTORDIALOG_P_H
#define DIFFSCOPE_UISHELL_MIDITRACKSELECTORDIALOG_P_H

#include <QCheckBox>
#include <QList>
#include <QScopedPointer>
#include <QString>

class QComboBox;
class QStandardItemModel;
class QTreeView;
class QTextEdit;

namespace UIShell {

    class MIDITrackSelectorStringConverter;

    class BinaryToggleCheckBox : public QCheckBox {
        Q_OBJECT
    public:
        using QCheckBox::QCheckBox;

    protected:
        void nextCheckState() override {
            if (checkState() == Qt::Checked) {
                setCheckState(Qt::Unchecked);
            } else {
                setCheckState(Qt::Checked);
            }
        }
    };

    class MIDITrackSelectorDialog;

    class MIDITrackSelectorDialogPrivate {
        Q_DECLARE_PUBLIC(MIDITrackSelectorDialog)
    public:
        explicit MIDITrackSelectorDialogPrivate(MIDITrackSelectorDialog *q, MIDITrackSelectorStringConverter *converter);
        ~MIDITrackSelectorDialogPrivate();

        void init();
        void populateCodecCombo();
        void rebuildModel();
        void updateNamesForCodec();
        void updatePreviewForRow(int row);
        void updateSelectedIndexes();
        void updateSelectAllCheckBox();
        QString decodeBytes(const QByteArray &bytes) const;
        QString decodedName(const QByteArray &bytes) const;
        QString decodeLyrics(const QList<QByteArray> &lyrics) const;
        QByteArray aggregateLyricsForDetection() const;
        void syncComboToCodec();

        MIDITrackSelectorDialog *q_ptr;
        QScopedPointer<MIDITrackSelectorStringConverter> converter;
        QList<MIDITrackSelectorDialog::TrackInfo> trackInfos;
        QByteArray currentCodec;
        QByteArray autoDetectedCodec;
        QList<int> selectedIndexesCache;

        QComboBox *codecComboBox = nullptr;
        QTreeView *trackView = nullptr;
        QStandardItemModel *model = nullptr;
        QTextEdit *previewEdit = nullptr;
        BinaryToggleCheckBox *selectAllCheckBox = nullptr;
    };

}

#endif //DIFFSCOPE_UISHELL_MIDITRACKSELECTORDIALOG_P_H
