#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_DSPXFILEIMPORTER_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_DSPXFILEIMPORTER_H

#include <importexportmanager/FileConverter.h>

namespace ImportExportManager::Internal {

    class DspxFileImporter : public FileConverter {
        Q_OBJECT
    public:
        explicit DspxFileImporter(QObject *parent = nullptr);
        ~DspxFileImporter() override;

        bool execImport(const QString &path, QDspx::Model &model, QWindow *window) override;
    };

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_DSPXFILEIMPORTER_H
