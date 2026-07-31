#ifndef LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEIMPORTER_H
#define LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEIMPORTER_H

#include <importexportmanager/FileConverter.h>

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPFileImporter : public ImportExportManager::FileConverter {
        Q_OBJECT
    public:
        explicit LibreSVIPFileImporter(QObject *parent = nullptr);
        ~LibreSVIPFileImporter() override;

        bool runPreExecCheck() override;
        bool execImport(const QString &path, opendspx::Model &model, QWindow *window) override;

    private:
        void refreshFormats();
    };

}

#endif //LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFILEIMPORTER_H
