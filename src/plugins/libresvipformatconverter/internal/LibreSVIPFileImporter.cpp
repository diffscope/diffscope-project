#include "LibreSVIPFileImporter.h"

#include <QtCore/QtGlobal>

#include <libresvipformatconverter/internal/prebuiltinformats.h>

namespace LibreSVIPFormatConverter::Internal {

    LibreSVIPFileImporter::LibreSVIPFileImporter(QObject *parent) : FileConverter(parent) {
        setName(tr("Multiple formats (import via LibreSVIP)"));
        setDescription(tr("Import project files of other editors (such as VOCALOID, OpenUtau, etc.) via LibreSVIP"));
        setFileDialogFilters(preBuiltInFormatFileDialogFilters());
        setMode(Import);
        setHeuristicPriority(Low);
        setHeuristicFilters(preBuiltInFormatHeuristicFilters());
    }

    LibreSVIPFileImporter::~LibreSVIPFileImporter() = default;

    bool LibreSVIPFileImporter::execImport(const QString &path, QDspx::Model &model, QWindow *window) {
        // TODO
        return true;
    }

}
