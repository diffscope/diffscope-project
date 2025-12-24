#ifndef MIDI_FORMAT_CONVERTER_MIDIFILEIMPORTER_H
#define MIDI_FORMAT_CONVERTER_MIDIFILEIMPORTER_H

#include <importexportmanager/FileConverter.h>

namespace MIDIFormatConverter::Internal {

    class MIDIFileImporter : public ImportExportManager::FileConverter {
        Q_OBJECT
    public:
        explicit MIDIFileImporter(QObject *parent = nullptr);
        ~MIDIFileImporter() override;

        bool execImport(const QString &path, QDspx::Model &model, QWindow *window) override;
    };

}

#endif //MIDI_FORMAT_CONVERTER_MIDIFILEIMPORTER_H
