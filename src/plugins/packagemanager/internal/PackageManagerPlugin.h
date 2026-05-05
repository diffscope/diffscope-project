#ifndef DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERPLUGIN_H
#define DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace PackageManager {

    class PackageManagerPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        PackageManagerPlugin();
        ~PackageManagerPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERPLUGIN_H
