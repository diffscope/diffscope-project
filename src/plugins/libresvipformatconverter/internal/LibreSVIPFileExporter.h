#ifndef LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEEXPORTER_H
#define LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEEXPORTER_H

#include <importexportmanager/FileConverter.h>

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPFileExporter : public ImportExportManager::FileConverter {
        Q_OBJECT
    public:
        explicit LibreSVIPFileExporter(QObject *parent = nullptr);
        ~LibreSVIPFileExporter() override;

        bool execExport(const QString &path, const QDspx::Model &model, QWindow *window) override;
    };

}

#endif //LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEEXPORTER_H
