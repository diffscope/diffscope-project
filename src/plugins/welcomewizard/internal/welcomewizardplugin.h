#ifndef DIFFSCOPE_WELCOMEWIZARD_WELCOMEWIZARDPLUGIN_H
#define DIFFSCOPE_WELCOMEWIZARD_WELCOMEWIZARDPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace WelcomeWizard {

    class WelcomeWizardPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        WelcomeWizardPlugin();
        ~WelcomeWizardPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    };

}

#endif //DIFFSCOPE_WELCOMEWIZARD_WELCOMEWIZARDPLUGIN_H