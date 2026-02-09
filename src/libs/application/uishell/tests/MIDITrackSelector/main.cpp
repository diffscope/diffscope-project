#include <QApplication>
#include <QDebug>
#include <QMessageBox>

#include <uishell/MIDITrackSelectorDialog.h>

using namespace UIShell;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MIDITrackSelectorDialog dialog;
    dialog.setWindowTitle("MIDI Track Selector Test");

    // Create test data with various scenarios
    QList<MIDITrackSelectorDialog::TrackInfo> trackInfoList;

    // Track 1: Vocal track with lyrics
    {
        MIDITrackSelectorDialog::TrackInfo track;
        track.name = "Vocal";
        track.rangeText = "C3 - G5";
        track.noteCount = 128;
        track.lyrics = {"Hello", "world", "this", "is", "a", "song"};
        track.disabled = false;
        track.selectedByDefault = true;
        trackInfoList.append(track);
    }

    // Track 2: Piano accompaniment
    {
        MIDITrackSelectorDialog::TrackInfo track;
        track.name = "Piano";
        track.rangeText = "A1 - C7";
        track.noteCount = 256;
        track.lyrics = {};
        track.disabled = false;
        track.selectedByDefault = false;
        trackInfoList.append(track);
    }

    // Track 3: Disabled track (tempo/control track)
    {
        MIDITrackSelectorDialog::TrackInfo track;
        track.name = "Tempo Track";
        track.rangeText = "";
        track.noteCount = 0;
        track.lyrics = {};
        track.disabled = true;
        track.selectedByDefault = false;
        trackInfoList.append(track);
    }

    // Track 4: Vocal harmony with Chinese lyrics (GB18030)
    {
        MIDITrackSelectorDialog::TrackInfo track;
        track.name = QByteArray("\xba\xcd\xc9\xf9");  // "和声" in GB18030
        track.rangeText = "E3 - D5";
        track.noteCount = 96;
        track.lyrics = {
            QByteArray("\xc4\xe3\xba\xc3"),  // "你好"
            QByteArray("\xca\xc0\xbd\xe7"),  // "世界"
            QByteArray("\xd5\xe2\xca\xc7"),  // "这是"
            QByteArray("\xb8\xe8\xb4\xca")   // "歌词"
        };
        track.disabled = false;
        track.selectedByDefault = true;
        // trackInfoList.append(track);
    }

    // Track 5: Japanese lyrics (Shift-JIS)
    {
        MIDITrackSelectorDialog::TrackInfo track;
        track.name = QByteArray("\x83\x7B\x81\x5B\x83\x4A\x83\x8B\x82\x51");  // "ボーカル1" in Shift-JIS
        track.rangeText = "D3 - F#5";
        track.noteCount = 142;
        track.lyrics = {
            QByteArray("\x82\xb1\x82\xf1\x82\xc9\x82\xbf\x82\xcd"),  // "こんにちは"
            QByteArray("\x82\xbb\x82\xea\x82\xcd"),                  // "それは"
            QByteArray("\x89\xcc")                                   // "歌"
        };
        track.disabled = false;
        track.selectedByDefault = false;
        // trackInfoList.append(track);
    }

    // Track 6: Bass with many notes
    {
        MIDITrackSelectorDialog::TrackInfo track;
        track.name = "Bass";
        track.rangeText = "E1 - G3";
        track.noteCount = 512;
        track.lyrics = {};
        track.disabled = false;
        track.selectedByDefault = false;
        trackInfoList.append(track);
    }

    // Track 7: Drum track (disabled - no pitched notes)
    {
        MIDITrackSelectorDialog::TrackInfo track;
        track.name = "Drums";
        track.rangeText = "N/A";
        track.noteCount = 384;
        track.lyrics = {};
        track.disabled = true;
        track.selectedByDefault = false;
        trackInfoList.append(track);
    }

    // Track 8: Vocal with UTF-8 encoded name and lyrics
    {
        MIDITrackSelectorDialog::TrackInfo track;
        track.name = "Vocal Lead 🎵";
        track.rangeText = "C4 - A5";
        track.noteCount = 175;
        track.lyrics = {"La", "la", "la", "ooh", "yeah"};
        track.disabled = false;
        track.selectedByDefault = true;
        trackInfoList.append(track);
    }

    dialog.setTrackInfoList(trackInfoList);
    dialog.detectCodec();

    // Show dialog and check result
    int result = dialog.exec();

    if (result == QDialog::Accepted) {
        auto selectedIndexes = dialog.selectedIndexes();
        QString message = QString("Selected %1 track(s):\n").arg(selectedIndexes.size());
        
        for (int idx : selectedIndexes) {
            if (idx >= 0 && idx < trackInfoList.size()) {
                message += QString("  - Track %1: %2 (%3 notes)\n")
                    .arg(idx)
                    .arg(QString::fromUtf8(trackInfoList[idx].name))
                    .arg(trackInfoList[idx].noteCount);
            }
        }
        
        QMessageBox::information(nullptr, "Selection Result", message);
        qDebug() << "Selected tracks:" << selectedIndexes;
        return 0;
    } else {
        qDebug() << "Dialog canceled";
        return 0;
    }
}
