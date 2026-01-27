#ifndef DIFFSCOPE_COREPLUGIN_COREPLUGIN_H
#define DIFFSCOPE_COREPLUGIN_COREPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Core::Internal {

    class CorePlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        CorePlugin();
        ~CorePlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QObject *remoteCommand(const QStringList &options, const QString &workingDirectory, const QStringList &args) override;

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

    private:
        void initializeSingletons();
        void initializeActions();
        void initializeSettings();
        void initializeWindows();
        void initializeBehaviorPreference();
        void initializeColorScheme();
        void initializeJumpList();
        void initializeHelpContents();
        void initializePropertyEditors();

        static void checkLastRun();
        static void checkPlugins();

    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREPLUGIN_H
