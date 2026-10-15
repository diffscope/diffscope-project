// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACEADDON_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACEADDON_H

#include <QPointer>

#include <CoreApi/windowinterface.h>

Q_MOC_INCLUDE(<coreplugin / internal / ProjectWindowWorkspaceManager.h>)
Q_MOC_INCLUDE(<coreplugin / internal / WorkspaceSession.h>)

namespace Core::Internal {
    class ProjectWindowWorkspaceManager;
    class ProjectWindowWorkspaceLayout;
    class WorkspaceSession;

    class WorkspaceAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(ProjectWindowWorkspaceManager *workspaceManager READ workspaceManager CONSTANT)
        Q_PROPERTY(WorkspaceSession *session READ session CONSTANT)
        Q_PROPERTY(QVariantList panelEntries READ panelEntries NOTIFY panelEntriesChanged)
        Q_PROPERTY(QVariantList newPanelEntries READ newPanelEntries NOTIFY panelEntriesChanged)
        Q_PROPERTY(QStringList registeredPanelIds READ registeredPanelIds NOTIFY registeredPanelIdsChanged)
    public:
        explicit WorkspaceAddOn(QObject *parent = nullptr);
        ~WorkspaceAddOn() override;
        void initialize() override;
        void extensionsInitialized() override;
        ProjectWindowWorkspaceManager *workspaceManager() const;
        WorkspaceSession *session() const;
        Q_INVOKABLE void saveCurrentLayout();
        Q_INVOKABLE void markNewPanelsPresented();
        Q_INVOKABLE void saveCustomLayoutFromJavaScript(const ProjectWindowWorkspaceLayout &layout, const QString &originName, const QString &name) const;
        Q_INVOKABLE void removeCustomLayoutFromJavaScript(const QString &name) const;
        Q_INVOKABLE void showWorkspaceLayoutCommand() const;
        void showWorkspaceCustomLayoutCommand(const ProjectWindowWorkspaceLayout &layout) const;
        QVariantList panelEntries() const;
        QVariantList newPanelEntries() const;
        QStringList registeredPanelIds() const;
    Q_SIGNALS:
        void panelEntriesChanged();
        void registeredPanelIdsChanged();

    private:
        ProjectWindowWorkspaceManager *m_workspaceManager;
        WorkspaceSession *m_session = nullptr;
        QPointer<QObject> m_helper;
        QPointer<QObject> m_actions;
        QStringList m_registeredPanelIds;
    };
}

#endif
