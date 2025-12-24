#include "MIDIFileExporter.h"

#include <QtCore/QtGlobal>

namespace MIDIFormatConverter::Internal {

    MIDIFileExporter::MIDIFileExporter(QObject *parent) : FileConverter(parent) {
        setName(tr("MIDI"));
        setDescription(tr("Export as Standard MIDI file"));
        setFileDialogFilters({tr("Standard MIDI File (*.mid *.midi *.smf)")});
        setMode(Export);
        setHeuristicFilters({QStringLiteral("*.mid"), QStringLiteral("*.midi"), QStringLiteral("*.smf")});
    }

    MIDIFileExporter::~MIDIFileExporter() = default;

    bool MIDIFileExporter::execExport(const QString &path, const QDspx::Model &model, QWindow *window) {
        // TODO
        return true;
    }

}
