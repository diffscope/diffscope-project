#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_IMPORTEXPORTMANAGERPLUGIN_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_IMPORTEXPORTMANAGERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace ImportExportManager::Internal {

    class ImportExportManagerPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        ImportExportManagerPlugin();
        ~ImportExportManagerPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_IMPORTEXPORTMANAGERPLUGIN_H
