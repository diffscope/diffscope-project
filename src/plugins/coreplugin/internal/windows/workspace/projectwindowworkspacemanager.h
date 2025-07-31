#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H

#include <QObject>

#include <coreplugin/internal/projectwindowworkspacelayout.h>

namespace Core::Internal {

    class ProjectWindowWorkspaceLayout;

    class ProjectWindowWorkspaceManager : public QObject {
        Q_OBJECT
    public:
        enum SpecialIndex {
            Default = -1,
            Invalid = -2,
        };

        explicit ProjectWindowWorkspaceManager(QObject *parent = nullptr);
        ~ProjectWindowWorkspaceManager() override;

        ProjectWindowWorkspaceLayout currentLayout() const;
        void setCurrentLayout(const ProjectWindowWorkspaceLayout &layout);

        static QList<ProjectWindowWorkspaceLayout> customLayouts();
        static void setCustomLayouts(const QList<ProjectWindowWorkspaceLayout> &layouts);

        int currentIndex() const;
        void setCurrentIndex(int index);

        static ProjectWindowWorkspaceLayout defaultLayout();

        void load();
        void save() const;

    signals:
        void currentLayoutChanged();
        void customLayoutsChanged();
        void currentIndexChanged();

    private:
        ProjectWindowWorkspaceLayout m_currentLayout;
        int m_currentIndex{Default};

    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H
