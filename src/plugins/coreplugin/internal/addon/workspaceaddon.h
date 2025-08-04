#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACEADDON_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACEADDON_H

#include <CoreApi/iwindow.h>

namespace Core::Internal {

    class ProjectWindowWorkspaceManager;
    class ProjectWindowWorkspaceLayout;

    class WorkspaceAddOn : public IWindowAddOn {
        Q_OBJECT
        Q_PROPERTY(ProjectWindowWorkspaceManager *workspaceManager READ workspaceManager CONSTANT)
    public:
        explicit WorkspaceAddOn(QObject *parent = nullptr);
        ~WorkspaceAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        ProjectWindowWorkspaceManager *workspaceManager() const;
        Q_INVOKABLE QVariantMap createDockingViewContents(int edge) const;
        Q_INVOKABLE void saveCustomLayoutFromJavaScript(const ProjectWindowWorkspaceLayout &layout, const QString &originName, const QString &name) const;
        Q_INVOKABLE void removeCustomLayoutFromJavaScript(const QString &name) const;

    private:
        ProjectWindowWorkspaceManager *m_workspaceManager;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_WORKSPACEADDON_H
