#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWDATA_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWDATA_H

#include <QObject>
#include <qqmlintegration.h>

#include <QAKQuick/quickactioncontext.h>

namespace Core::Internal {

    class ProjectWindowWorkspaceManager;

    class ProjectWindowData : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAK::QuickActionContext *actionContext READ actionContext CONSTANT)
        Q_PROPERTY(ProjectWindowWorkspaceManager *workspaceManager READ workspaceManager CONSTANT)
    public:
        explicit ProjectWindowData(QObject *parent = nullptr);
        ~ProjectWindowData() override;
        QAK::QuickActionContext *actionContext() const;
        ProjectWindowWorkspaceManager *workspaceManager() const;

        enum PanelPosition {
            Windowed,
            LeftTop,
            LeftBottom,
            RightTop,
            RightBottom,
            TopLeft,
            TopRight,
            BottomLeft,
            BottomRight,
        };
        Q_ENUM(PanelPosition)

        Q_INVOKABLE QVariantMap createDockingViewContents(int edge) const;

    signals:
        void initialized();

    private:
        QAK::QuickActionContext *m_actionContext;
        ProjectWindowWorkspaceManager *m_workspaceManager;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWDATA_H
