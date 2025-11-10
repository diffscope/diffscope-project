#ifndef DIFFSCOPE_VISUAL_EDITOR_VISUALEDITORPLUGIN_H
#define DIFFSCOPE_VISUAL_EDITOR_VISUALEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace VisualEditor::Internal {

    class VisualEditorPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        VisualEditorPlugin();
        ~VisualEditorPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_VISUALEDITORPLUGIN_H
