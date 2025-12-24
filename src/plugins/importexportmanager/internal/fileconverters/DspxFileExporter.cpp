#include "DspxFileExporter.h"

namespace ImportExportManager::Internal {

    DspxFileExporter::DspxFileExporter(QObject *parent) : FileConverter(parent) {
        setName(tr("DSPX"));
        setDescription(tr("Export as DSPX file compatible with older versions of DiffScope and other editors"));
        setFileDialogFilters({tr("DiffScope Project Exchange Format (*.dspx)")});
        setMode(Export);
        setHeuristicFilters({QStringLiteral("*.dspx")});
    }

    DspxFileExporter::~DspxFileExporter() = default;

    bool DspxFileExporter::execExport(const QString &path, const QDspx::Model &model, QWindow *window) {
        // TODO
        return true;
    }

}
