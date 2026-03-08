#include "MIDIFileImporter.h"

#include <algorithm>
#include <iterator>

#include <QLoggingCategory>
#include <QFile>
#include <QDir>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/MusicPitch.h>
#include <SVSCraftQuick/MessageBox.h>

#include <opendspx/model.h>
#include <opendspxconverter/midi/midiconverter.h>
#include <opendspxconverter/midi/midiintermediatedata.h>

#include <midiformatconverter/internal/MIDITrackSelectorDialog.h>
#include <midiformatconverter/internal/MIDITextCodecConverter.h>

namespace MIDIFormatConverter::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcMIDIFileImporter, "diffscope.midiformatconverter.midifileimporter")

    MIDIFileImporter::MIDIFileImporter(QObject *parent) : FileConverter(parent) {
        setName(tr("MIDI"));
        setDescription(tr("Import Standard MIDI file"));
        setFileDialogFilters({tr("Standard MIDI File (*.mid *.midi *.smf)")});
        setMode(Import);
        setHeuristicFilters({QStringLiteral("*.mid"), QStringLiteral("*.midi"), QStringLiteral("*.smf")});
    }

    MIDIFileImporter::~MIDIFileImporter() = default;

    static QList<MIDITrackSelectorDialog::TrackInfo> getTrackInfoList(const QList<QDspx::MidiIntermediateData::Track> &tracks) {
        QList<MIDITrackSelectorDialog::TrackInfo> result;
        for (const auto &track : tracks) {
            MIDITrackSelectorDialog::TrackInfo info;
            info.name = track.title;
            auto minMaxNotes = std::ranges::minmax_element(track.notes, [](const auto &a, const auto &b) { return a.key < b.key; });
            info.rangeText = track.notes.isEmpty() ? QString() : QStringLiteral("%1 - %2").arg(SVS::MusicPitch(static_cast<qint8>(minMaxNotes.min->key)).toString(SVS::MusicPitch::Flat), SVS::MusicPitch(static_cast<qint8>(minMaxNotes.max->key)).toString(SVS::MusicPitch::Flat));
            info.noteCount = static_cast<int>(track.notes.size());
            info.selectedByDefault = !track.notes.isEmpty();
            std::ranges::transform(track.notes, std::back_inserter(info.lyrics), [](const auto &note) { return note.lyric; });
            result.append(info);
        }
        return result;
    }

    bool MIDIFileImporter::execImport(const QString &path, QDspx::Model &model, QWindow *window) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCCritical(lcMIDIFileImporter) << "Failed to read file:" << path << file.errorString();
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to open file"), QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), file.errorString()));
            return false;
        }
        auto data = file.readAll();
        QDspx::MidiConverter::Error error;
        auto intermediateData = QDspx::MidiConverter::convertMidiToIntermediate(data, error);
        if (error == QDspx::MidiConverter::Error::InvalidMidiData) {
            qCCritical(lcMIDIFileImporter) << "Invalid MIDI data:" << path;
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Invalid MIDI data"), tr("The file is not a valid MIDI file"));
            return false;
        }
        if (error == QDspx::MidiConverter::Error::UnsupportedFormat) {
            qCCritical(lcMIDIFileImporter) << "Unsupported MIDI format:" << path;
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Unsupported MIDI format"), tr("The file is not a supported MIDI format"));
            return false;
        }
        MIDITrackSelectorDialog dlg;
        dlg.setTrackInfoList(getTrackInfoList(intermediateData.tracks()));
        dlg.detectCodec();
        connect(&dlg, &MIDITrackSelectorDialog::separateMidiChannelsChanged, [&](bool enabled) {
            auto intermediateData_ = QDspx::MidiConverter::convertMidiToIntermediate(data, error, { enabled });
            if (error == QDspx::MidiConverter::Error::NoError) {
                intermediateData = intermediateData_;
                dlg.setTrackInfoList(getTrackInfoList(intermediateData.tracks()));
                dlg.detectCodec();
            }
        });
        if (dlg.exec() != QDialog::Accepted) {
            return false;
        }
        auto codec = dlg.codec();
        auto selectedIndexes = dlg.selectedIndexes();
        bool ok;
        QList<QDspx::MidiIntermediateData::Track> selectedTracks;
        for (auto index : selectedIndexes) {
            selectedTracks.append(intermediateData.tracks().at(index));
        }
        intermediateData = {
            intermediateData.resolution(),
            dlg.importTempo() ? intermediateData.tempos() : QList<QDspx::MidiIntermediateData::Tempo>{},
            dlg.importTimeSignature() ? intermediateData.timeSignatures() : QList<QDspx::MidiIntermediateData::TimeSignature>{},
            intermediateData.markers(),
            selectedTracks
        };
        model = QDspx::MidiConverter::convertIntermediateToDspx(intermediateData, [codec](const QByteArray &text) {
            return MIDITextCodecConverter::decode(text, codec);
        }, &ok);
        if (!ok) {
            qCCritical(lcMIDIFileImporter) << "Failed to convert MIDI data:" << path;
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to convert MIDI data"), tr("Some meta events in this MIDI document cannot be converted to DSPX. Please try disabling import tempo/time signature."));
            return false;
        }
        return true;
    }

}
