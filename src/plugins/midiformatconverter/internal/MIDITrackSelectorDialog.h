#ifndef DIFFSCOPE_MIDI_FORMAT_CONVERTER_MIDITRACKSELECTORDIALOG_H
#define DIFFSCOPE_MIDI_FORMAT_CONVERTER_MIDITRACKSELECTORDIALOG_H

#include <QByteArray>
#include <QDialog>
#include <QList>
#include <QScopedPointer>
#include <QString>

namespace MIDIFormatConverter::Internal {

    class MIDITrackSelectorDialogPrivate;

    class MIDITrackSelectorDialog : public QDialog {
        Q_OBJECT
        Q_DECLARE_PRIVATE(MIDITrackSelectorDialog)

        Q_PROPERTY(QByteArray codec READ codec WRITE setCodec NOTIFY codecChanged)
        Q_PROPERTY(QList<int> selectedIndexes READ selectedIndexes NOTIFY selectedIndexesChanged)
        Q_PROPERTY(bool separateMidiChannels READ separateMidiChannels WRITE setseparateMidiChannels NOTIFY separateMidiChannelsChanged)
        Q_PROPERTY(bool importTempo READ importTempo WRITE setImportTempo NOTIFY importTempoChanged)
        Q_PROPERTY(bool importTimeSignature READ importTimeSignature WRITE setImportTimeSignature NOTIFY importTimeSignatureChanged)

    public:
        struct TrackInfo {
            QByteArray name;
            QString rangeText;
            int noteCount = 0;
            QList<QByteArray> lyrics;
            bool disabled = false;
            bool selectedByDefault = false;
        };

        explicit MIDITrackSelectorDialog(QWidget *parent = nullptr);
        ~MIDITrackSelectorDialog() override;

        QList<TrackInfo> trackInfoList() const;
        void setTrackInfoList(const QList<TrackInfo> &trackInfoList);

        QByteArray codec() const;
        void setCodec(const QByteArray &codec);

        QList<int> selectedIndexes() const;

        bool separateMidiChannels() const;
        void setseparateMidiChannels(bool enabled);

        bool importTempo() const;
        void setImportTempo(bool enabled);

        bool importTimeSignature() const;
        void setImportTimeSignature(bool enabled);

        Q_INVOKABLE void detectCodec();

    Q_SIGNALS:
        void codecChanged(const QByteArray &codec);
        void selectedIndexesChanged();
        void separateMidiChannelsChanged(bool enabled);
        void importTempoChanged(bool enabled);
        void importTimeSignatureChanged(bool enabled);

    private:
        QScopedPointer<MIDITrackSelectorDialogPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_MIDI_FORMAT_CONVERTER_MIDITRACKSELECTORDIALOG_H
