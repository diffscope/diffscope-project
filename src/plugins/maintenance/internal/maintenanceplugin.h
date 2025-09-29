#ifndef DIFFSCOPE_MAINTENANCE_MAINTENANCEPLUGIN_H
#define DIFFSCOPE_MAINTENANCE_MAINTENANCEPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Maintenance {

    class MaintenancePlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        MaintenancePlugin();
        ~MaintenancePlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    };

}

#endif //DIFFSCOPE_MAINTENANCE_MAINTENANCEPLUGIN_H
