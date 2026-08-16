// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "MIDIFileImporter.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <utility>

#include <QLoggingCategory>
#include <QFile>
#include <QDir>

#include <CoreApi/runtimeinterface.h>

#include <stdcorelib/support/json.h>

#include <SVSCraftCore/MusicPitch.h>
#include <SVSCraftCore/MusicMode.h>
#include <SVSCraftQuick/MessageBox.h>

#include <opendspx/model.h>
#include <opendspx/converter/midi/midiconverter.h>
#include <opendspx/converter/midi/midiintermediatedata.h>

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

    static QList<MIDITrackSelectorDialog::TrackInfo> getTrackInfoList(const std::vector<opendspx::MidiIntermediateData::Track> &tracks) {
        QList<MIDITrackSelectorDialog::TrackInfo> result;
        for (const auto &track : tracks) {
            MIDITrackSelectorDialog::TrackInfo info;
            info.name = QByteArray::fromStdString(track.title);
            auto minMaxNotes = std::ranges::minmax_element(track.notes, [](const auto &a, const auto &b) { return a.key < b.key; });
            info.rangeText = track.notes.empty() ? QString() : QStringLiteral("%1 - %2").arg(SVS::MusicPitch(static_cast<qint8>(minMaxNotes.min->key)).toString(SVS::MusicPitch::Flat), SVS::MusicPitch(static_cast<qint8>(minMaxNotes.max->key)).toString(SVS::MusicPitch::Flat));
            info.noteCount = static_cast<int>(track.notes.size());
            info.selectedByDefault = !track.notes.empty();
            std::ranges::transform(track.notes, std::back_inserter(info.lyrics), [](const auto &note) { return QByteArray::fromStdString(note.lyric); });
            result.append(info);
        }
        return result;
    }

    static void postprocessImportedModel(opendspx::Model &model) {
        for (auto &track : model.content.tracks) {
            for (const auto &clip : track.clips) {
                if (!clip || clip->type != opendspx::Clip::Type::Singing || !clip->name.empty())
                    continue;

                const auto singingClip = std::static_pointer_cast<opendspx::SingingClip>(clip);
                const auto lyricCount = std::min<std::size_t>(8, singingClip->notes.size());
                for (std::size_t i = 0; i < lyricCount; ++i) {
                    if (i != 0)
                        clip->name.push_back(' ');
                    clip->name.append(singingClip->notes[i].lyric);
                }
            }

            if (track.name.empty() && !track.clips.empty() && track.clips.front()) {
                track.name = track.clips.front()->name;
            }
        }
    }

    static void addDetectedKeySignature(opendspx::Model &model, int mode, int accidentalType, const QList<SVS::MusicPitch> &notes) {
        if (notes.isEmpty())
            return;

        const auto tonality = SVS::MusicMode(mode).detectTonality(notes);
        stdc::JsonObject keySignature;
        keySignature["pos"] = 0;
        keySignature["mode"] = mode;
        keySignature["tonality"] = static_cast<int>(tonality);
        keySignature["accidentalType"] = accidentalType;

        auto &keySignatures = model.content.workspace["diffscope"]["keySignatures"];
        stdc::JsonArray keySignatureArray = keySignatures.isArray() ? keySignatures.toArray() : stdc::JsonArray{};
        keySignatureArray.push_back(std::move(keySignature));
        keySignatures = std::move(keySignatureArray);
    }

    bool MIDIFileImporter::execImport(const QString &path, opendspx::Model &model, QWindow *window) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCCritical(lcMIDIFileImporter) << "Failed to read file:" << path << file.errorString();
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to open file"), QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), file.errorString()));
            return false;
        }
        const auto data = file.readAll().toStdString();
        std::stringstream in(data, std::ios::in);
        opendspx::MidiConverter::Error error;
        auto intermediateData = opendspx::MidiConverter::convertMidiToIntermediate(in, error);
        if (error == opendspx::MidiConverter::Error::InvalidMidiData) {
            qCCritical(lcMIDIFileImporter) << "Invalid MIDI data:" << path;
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Invalid MIDI data"), tr("The file is not a valid MIDI file"));
            return false;
        }
        if (error == opendspx::MidiConverter::Error::UnsupportedFormat) {
            qCCritical(lcMIDIFileImporter) << "Unsupported MIDI format:" << path;
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Unsupported MIDI format"), tr("The file is not a supported MIDI format"));
            return false;
        }
        MIDITrackSelectorDialog dlg;
        dlg.setTrackInfoList(getTrackInfoList(intermediateData.tracks()));
        dlg.detectCodec();
        connect(&dlg, &MIDITrackSelectorDialog::separateMidiChannelsChanged, [&](bool enabled) {
            std::stringstream in_(data, std::ios::in);
            auto intermediateData_ = opendspx::MidiConverter::convertMidiToIntermediate(in_, error, { enabled });
            if (error == opendspx::MidiConverter::Error::NoError) {
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
        const bool autoDetectKeySignature = dlg.autoDetectKeySignature();
        const int musicMode = dlg.musicMode();
        const int accidentalType = dlg.accidentalType();
        bool ok;
        auto sourceTracks = intermediateData.tracks();
        std::vector<opendspx::MidiIntermediateData::Track> selectedTracks;
        QList<SVS::MusicPitch> selectedNotes;
        for (auto index : selectedIndexes) {
            auto &track = sourceTracks.at(index);
            if (autoDetectKeySignature) {
                selectedNotes.reserve(selectedNotes.size() + static_cast<qsizetype>(track.notes.size()));
                for (const auto &note : track.notes) {
                    selectedNotes.append(SVS::MusicPitch(static_cast<qint8>(note.key)));
                }
            }
            selectedTracks.push_back(std::move(track));
        }
        intermediateData = {
            intermediateData.resolution(),
            dlg.importTempo() ? intermediateData.tempos() : std::vector<opendspx::MidiIntermediateData::Tempo>{},
            dlg.importTimeSignature() ? intermediateData.timeSignatures() : std::vector<opendspx::MidiIntermediateData::TimeSignature>{},
            intermediateData.markers(),
            selectedTracks
        };
        model = opendspx::MidiConverter::convertIntermediateToDspx(intermediateData, [codec](const std::string &data) {
            return MIDITextCodecConverter::decode(QByteArray::fromStdString(data), codec).toStdString();
        }, &ok);
        if (!ok) {
            qCCritical(lcMIDIFileImporter) << "Failed to convert MIDI data:" << path;
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to convert MIDI data"), tr("Some meta events in this MIDI document cannot be converted to DSPX. Please try disabling import tempo/time signature."));
            return false;
        }
        postprocessImportedModel(model);
        if (autoDetectKeySignature) {
            addDetectedKeySignature(model, musicMode, accidentalType, selectedNotes);
        }
        return true;
    }

}
