#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_IMPORTADDON_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_IMPORTADDON_H

#include <CoreApi/windowinterface.h>

namespace ImportExportManager::Internal {

    class ImportAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit ImportAddOn(QObject *parent = nullptr);
        ~ImportAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_IMPORTADDON_H