#ifndef CONVERTERCOLLECTION_H
#define CONVERTERCOLLECTION_H

#include <CoreApi/objectpool.h>

#include <importexportmanager/importexportmanagerglobal.h>

namespace ImportExportManager {

    namespace Internal {
        class ImportExportManagerPlugin;
    }

    class FileConverter;

    class IMPORT_EXPORT_MANAGER_EXPORT ConverterCollection : public Core::ObjectPool {
        Q_OBJECT
    public:
        ~ConverterCollection() override;

        static ConverterCollection *instance();

        QList<FileConverter *> fileConverters() const;

    private:
        friend class Internal::ImportExportManagerPlugin;
        explicit ConverterCollection(QObject *parent);

    };

}

#endif //CONVERTERCOLLECTION_H
