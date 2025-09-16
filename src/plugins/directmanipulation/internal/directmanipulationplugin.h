#ifndef DIFFSCOPE_DIRECT_MANIPULATION_DIRECTMANIPULATIONPLUGIN_H
#define DIFFSCOPE_DIRECT_MANIPULATION_DIRECTMANIPULATIONPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace DirectManipulation::Internal {

    class DirectManipulationPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        DirectManipulationPlugin();
        ~DirectManipulationPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;

    };

}

#endif //DIFFSCOPE_DIRECT_MANIPULATION_DIRECTMANIPULATIONPLUGIN_H
