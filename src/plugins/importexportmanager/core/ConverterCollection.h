#ifndef CONVERTERCOLLECTION_H
#define CONVERTERCOLLECTION_H

#include <QObject>

#include <importexportmanager/importexportmanagerglobal.h>

namespace ImportExportManager {

    namespace Internal {
        class ImportExportManagerPlugin;
    }

    class FileConverter;

    class IMPORT_EXPORT_MANAGER_EXPORT ConverterCollection : public QObject {
        Q_OBJECT
    public:
        ~ConverterCollection() override;

        static ConverterCollection *instance();

        static QList<FileConverter *> fileConverters();
        static void addFileConverter(FileConverter *converter);

    private:
        friend class Internal::ImportExportManagerPlugin;
        explicit ConverterCollection(QObject *parent);

    };

}

#endif //CONVERTERCOLLECTION_H
