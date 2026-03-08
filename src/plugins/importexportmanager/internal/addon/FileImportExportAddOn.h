#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILEIMPORTEXPORTADDON_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILEIMPORTEXPORTADDON_H

#include <CoreApi/windowinterface.h>

namespace ImportExportManager {
    class FileConverter;
}

namespace ImportExportManager::Internal {

    class FileImportExportAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(QList<FileConverter *> importConverters READ importConverters CONSTANT)
        Q_PROPERTY(QList<FileConverter *> exportConverters READ exportConverters CONSTANT)
        Q_PROPERTY(bool isHomeWindow READ isHomeWindow CONSTANT)
    public:
        explicit FileImportExportAddOn(QObject *parent = nullptr);
        ~FileImportExportAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static QList<FileConverter *> importConverters(const QString &path = {});
        static QList<FileConverter *> exportConverters();
        Q_INVOKABLE void execImport(FileConverter *converter) const;
        Q_INVOKABLE void execExport(FileConverter *converter) const;

        bool isHomeWindow() const;
    };

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILEIMPORTEXPORTADDON_H