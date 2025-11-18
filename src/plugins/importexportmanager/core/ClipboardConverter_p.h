#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_CLIPBOARDCONVERTER_P_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_CLIPBOARDCONVERTER_P_H

#include <importexportmanager/ClipboardConverter.h>

namespace ImportExportManager {

    class ClipboardConverterPrivate {
        Q_DECLARE_PUBLIC(ClipboardConverter)
    public:
        ClipboardConverter *q_ptr;
        
        QString name;
        QString description;
        QStringList mimeTypes;
        ClipboardConverter::Modes modes{};
    };

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_CLIPBOARDCONVERTER_P_H
