#include "DspxFileImporter.h"

namespace ImportExportManager::Internal {

    DspxFileImporter::DspxFileImporter(QObject *parent) : FileConverter(parent) {
        setName(tr("DSPX"));
        setDescription(tr("Import DSPX file compatible with DiffScope"));
        setFileDialogFilters({tr("DiffScope Project Exchange Format (*.dspx)")});
        setMode(Import);
        setHeuristicFilters({QStringLiteral("*.dspx")});
    }

    DspxFileImporter::~DspxFileImporter() = default;

    bool DspxFileImporter::execImport(const QString &path, QDspx::Model &model, QWindow *window) {
        // TODO
        return true;
    }

}
