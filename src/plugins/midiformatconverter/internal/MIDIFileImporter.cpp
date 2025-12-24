#include "MIDIFileImporter.h"

#include <QtCore/QtGlobal>

namespace MIDIFormatConverter::Internal {

    MIDIFileImporter::MIDIFileImporter(QObject *parent) : FileConverter(parent) {
        setName(tr("MIDI"));
        setDescription(tr("Import Standard MIDI file"));
        setFileDialogFilters({tr("Standard MIDI File (*.mid *.midi *.smf)")});
        setMode(Import);
        setHeuristicFilters({QStringLiteral("*.mid"), QStringLiteral("*.midi"), QStringLiteral("*.smf")});
    }

    MIDIFileImporter::~MIDIFileImporter() = default;

    bool MIDIFileImporter::execImport(const QString &path, QDspx::Model &model, QWindow *window) {
        // TODO
        return true;
    }

}
