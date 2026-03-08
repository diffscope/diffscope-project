#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILECONVERTER_P_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILECONVERTER_P_H

#include <importexportmanager/FileConverter.h>

namespace ImportExportManager {

    class FileConverterPrivate {
        Q_DECLARE_PUBLIC(FileConverter)
    public:
        FileConverter *q_ptr;
        
        QString name;
        QString description;
        QStringList fileDialogFilters;
        FileConverter::Mode mode{FileConverter::Import};
        FileConverter::HeuristicPriority heuristicPriority{FileConverter::Normal};
        QStringList heuristicFilters;
    };

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILECONVERTER_P_H
