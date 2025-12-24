#ifndef MIDI_FORMAT_CONVERTER_MIDIFILEEXPORTER_H
#define MIDI_FORMAT_CONVERTER_MIDIFILEEXPORTER_H

#include <importexportmanager/FileConverter.h>

namespace MIDIFormatConverter::Internal {

    class MIDIFileExporter : public ImportExportManager::FileConverter {
        Q_OBJECT
    public:
        explicit MIDIFileExporter(QObject *parent = nullptr);
        ~MIDIFileExporter() override;

        bool execExport(const QString &path, const QDspx::Model &model, QWindow *window) override;
    };

}

#endif //MIDI_FORMAT_CONVERTER_MIDIFILEEXPORTER_H
