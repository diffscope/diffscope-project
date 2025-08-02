#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWDATA_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWDATA_H

#include <QObject>
#include <QWindow>
#include <qqmlintegration.h>

#include <QAKQuick/quickactioncontext.h>

namespace Core::Internal {

    class ProjectWindowWorkspaceManager;

    class ProjectWindowData : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QWindow *window READ window CONSTANT)
        Q_PROPERTY(QAK::QuickActionContext *actionContext READ actionContext CONSTANT)
        Q_PROPERTY(ProjectWindowWorkspaceManager *workspaceManager READ workspaceManager CONSTANT)
        Q_PROPERTY(ProjectWindowData::PanelPosition activePanel READ activePanel WRITE setActivePanel NOTIFY activePanelChanged)
        Q_PROPERTY(QObject *activeUndockedPane READ activeUndockedPane WRITE setActiveUndockedPane NOTIFY activeUndockedPaneChanged)
        Q_PROPERTY(QObject *activeDockingPane READ activeDockingPane WRITE setActiveDockingPane NOTIFY activeDockingPaneChanged)
        Q_PROPERTY(QObjectList dockingPanes READ dockingPanes WRITE setDockingPanes NOTIFY dockingPanesChanged)
        Q_PROPERTY(QObjectList floatingPanes READ floatingPanes WRITE setFloatingPanes NOTIFY floatingPanesChanged)
    public:
        explicit ProjectWindowData(QObject *parent = nullptr);
        ~ProjectWindowData() override;

        inline QWindow *window() const {
            return qobject_cast<QWindow *>(parent());
        }

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

        PanelPosition activePanel() const;
        void setActivePanel(PanelPosition panel);

        QObject *activeUndockedPane() const;
        void setActiveUndockedPane(QObject *undockedPane);

        QObject *activeDockingPane() const;
        void setActiveDockingPane(QObject *dockingPane);

        QObjectList dockingPanes() const;
        void setDockingPanes(const QObjectList &dockingPanes);

        QObjectList floatingPanes() const;
        void setFloatingPanes(const QObjectList &floatingPanes);

        Q_INVOKABLE QVariantMap createDockingViewContents(int edge) const;

    signals:
        void initialized();
        void activePanelChanged();
        void activeUndockedPaneChanged();
        void activeDockingPaneChanged();
        void dockingPanesChanged();
        void floatingPanesChanged();

    private:
        QAK::QuickActionContext *m_actionContext;
        ProjectWindowWorkspaceManager *m_workspaceManager;
        PanelPosition m_activePanel{};
        QObject *m_activeUndockedPane{};
        QObject *m_activeDockingPane{};
        QObjectList m_dockingPanes;
        QObjectList m_floatingPanes;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWDATA_H
