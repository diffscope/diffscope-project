// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H

#include <QObject>

#include <coreplugin/internal/ProjectWindowWorkspaceLayout.h>

namespace Core::Internal {

    class ProjectWindowWorkspaceLayout;

    class ProjectWindowWorkspaceManager : public QObject {
        Q_OBJECT
        Q_PROPERTY(ProjectWindowWorkspaceLayout currentLayout READ currentLayout NOTIFY currentLayoutChanged)
        Q_PROPERTY(QList<ProjectWindowWorkspaceLayout> customLayouts READ customLayouts WRITE setCustomLayouts NOTIFY customLayoutsChanged)
    public:
        explicit ProjectWindowWorkspaceManager(QObject *parent = nullptr);
        ~ProjectWindowWorkspaceManager() override;

        ProjectWindowWorkspaceLayout currentLayout() const;
        void setCurrentLayoutSnapshot(const ProjectWindowWorkspaceLayout &layout);
        Q_INVOKABLE void applyDefaultLayout();
        Q_INVOKABLE void applyCustomLayout(const ProjectWindowWorkspaceLayout &layout);

        static QList<ProjectWindowWorkspaceLayout> customLayouts();
        static void setCustomLayouts(const QList<ProjectWindowWorkspaceLayout> &layouts);

        void load();
        void save() const;

    Q_SIGNALS:
        void currentLayoutChanged();
        void customLayoutsChanged();

    private:
        static ProjectWindowWorkspaceLayout defaultLayout();
        void setCurrentLayout(const ProjectWindowWorkspaceLayout &layout, bool isCustom);

        ProjectWindowWorkspaceLayout m_currentLayout;
        bool m_currentLayoutIsCustom{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H
