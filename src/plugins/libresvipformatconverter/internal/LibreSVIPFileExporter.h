#ifndef LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEEXPORTER_H
#define LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEEXPORTER_H

#include <importexportmanager/FileConverter.h>

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPFileExporter : public ImportExportManager::FileConverter {
        Q_OBJECT
    public:
        explicit LibreSVIPFileExporter(QObject *parent = nullptr);
        ~LibreSVIPFileExporter() override;

        bool runPreExecCheck() override;
        bool execExport(const QString &path, const opendspx::Model &model, QWindow *window) override;

    private:
        void refreshFormats();
    };

}

#endif //LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEEXPORTER_H
