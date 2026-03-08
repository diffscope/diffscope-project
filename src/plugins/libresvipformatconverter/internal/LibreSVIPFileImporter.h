#ifndef LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEIMPORTER_H
#define LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEIMPORTER_H

#include <importexportmanager/FileConverter.h>

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPFileImporter : public ImportExportManager::FileConverter {
        Q_OBJECT
    public:
        explicit LibreSVIPFileImporter(QObject *parent = nullptr);
        ~LibreSVIPFileImporter() override;

        bool execImport(const QString &path, QDspx::Model &model, QWindow *window) override;
    };

}

#endif //LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEIMPORTER_H
