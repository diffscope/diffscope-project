#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACEADDON_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACEADDON_H

#include <QPointer>

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class ProjectWindowWorkspaceManager;
    class ProjectWindowWorkspaceLayout;

    class WorkspaceAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(ProjectWindowWorkspaceManager *workspaceManager READ workspaceManager CONSTANT)
        Q_PROPERTY(QVariantList panelEntries READ panelEntries NOTIFY panelEntriesChanged)
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

        Q_INVOKABLE void showWorkspaceLayoutCommand() const;
        void showWorkspaceCustomLayoutCommand(const ProjectWindowWorkspaceLayout &layout) const;

        QVariantList panelEntries() const;

        bool eventFilter(QObject *watched, QEvent *event) override;

    Q_SIGNALS:
        void panelEntriesChanged();
        void windowExposed();

    private:
        ProjectWindowWorkspaceManager *m_workspaceManager;
        QPointer<QObject> m_helper;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_WORKSPACEADDON_H
