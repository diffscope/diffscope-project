#include "LibreSVIPFileExporter.h"

#include <QtCore/QtGlobal>

#include <libresvipformatconverter/internal/prebuiltinformats.h>

namespace LibreSVIPFormatConverter::Internal {

    LibreSVIPFileExporter::LibreSVIPFileExporter(QObject *parent) : FileConverter(parent) {
        setName(tr("Multiple formats (export via LibreSVIP)"));
        setDescription(tr("Export as project files of other editors (such as VOCALOID, OpenUtau, etc.) via LibreSVIP"));
        setFileDialogFilters(preBuiltInFormatFileDialogFilters());
        setMode(Export);
        setHeuristicPriority(Low);
        setHeuristicFilters(preBuiltInFormatHeuristicFilters());
    }

    LibreSVIPFileExporter::~LibreSVIPFileExporter() = default;

    bool LibreSVIPFileExporter::execExport(const QString &path, const QDspx::Model &model, QWindow *window) {
        // TODO
        return true;
    }

}
