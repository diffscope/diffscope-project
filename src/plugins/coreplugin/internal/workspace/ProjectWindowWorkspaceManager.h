#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H

#include <QObject>

#include <coreplugin/internal/ProjectWindowWorkspaceLayout.h>

namespace Core::Internal {

    class ProjectWindowWorkspaceLayout;

    class ProjectWindowWorkspaceManager : public QObject {
        Q_OBJECT
        Q_PROPERTY(ProjectWindowWorkspaceLayout currentLayout READ currentLayout WRITE setCurrentLayout NOTIFY currentLayoutChanged)
        Q_PROPERTY(QList<ProjectWindowWorkspaceLayout> customLayouts READ customLayouts WRITE setCustomLayouts NOTIFY customLayoutsChanged)
        Q_PROPERTY(ProjectWindowWorkspaceLayout defaultLayout READ defaultLayout CONSTANT)
    public:
        explicit ProjectWindowWorkspaceManager(QObject *parent = nullptr);
        ~ProjectWindowWorkspaceManager() override;

        ProjectWindowWorkspaceLayout currentLayout() const;
        void setCurrentLayout(const ProjectWindowWorkspaceLayout &layout);

        static QList<ProjectWindowWorkspaceLayout> customLayouts();
        static void setCustomLayouts(const QList<ProjectWindowWorkspaceLayout> &layouts);

        static ProjectWindowWorkspaceLayout defaultLayout();

        void load();
        void save() const;

    signals:
        void currentLayoutChanged();
        void customLayoutsChanged();

    private:
        ProjectWindowWorkspaceLayout m_currentLayout;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H
