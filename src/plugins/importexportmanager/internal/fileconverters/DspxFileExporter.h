#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_DSPXFILEEXPORTER_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_DSPXFILEEXPORTER_H

#include <importexportmanager/FileConverter.h>

namespace ImportExportManager::Internal {

    class DspxFileExporter : public FileConverter {
        Q_OBJECT
    public:
        explicit DspxFileExporter(QObject *parent = nullptr);
        ~DspxFileExporter() override;

        bool execExport(const QString &path, const QDspx::Model &model, QWindow *window) override;
    };

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_DSPXFILEEXPORTER_H
