// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACESESSION_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACESESSION_H

#include <functional>

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QVariantList>

#include <coreplugin/internal/ProjectWindowWorkspaceLayout.h>

Q_MOC_INCLUDE(<SVSCraftQuick / DockingContext.h>)
Q_MOC_INCLUDE(<SVSCraftQuick / DockingHost.h>)

class QQuickItem;
class QQuickWindow;

namespace SVS {
    class DockingContext;
    class DockingHost;
    class DockingPanel;
    class DockingRequest;
}

namespace Core {
    class ProjectWindowInterface;

    namespace Internal {
        class WorkspacePanelLoader;

        class WorkspaceSession : public QObject {
            Q_OBJECT
            Q_PROPERTY(SVS::DockingContext *context READ context CONSTANT)
            Q_PROPERTY(SVS::DockingHost *embeddedHost READ embeddedHost CONSTANT)
            Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
            Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
            Q_PROPERTY(bool shuttingDown READ isShuttingDown NOTIFY shuttingDownChanged)
        public:
            explicit WorkspaceSession(ProjectWindowInterface *window, QObject *parent = nullptr);
            ~WorkspaceSession() override;
            SVS::DockingContext *context() const;
            SVS::DockingHost *embeddedHost() const;
            bool isBusy() const;
            bool isReady() const;
            bool isShuttingDown() const;
            void initialize();
            void apply(const ProjectWindowWorkspaceLayout &layout);
            ProjectWindowWorkspaceLayout snapshot(const QString &name) const;
            Q_INVOKABLE void showPanel(const QString &id);
            void shutdown();
        Q_SIGNALS:
            void busyChanged();
            void readyChanged();
            void shuttingDownChanged();
            void restored();
            void errorOccurred(const QString &message);

        private:
            SVS::DockingPanel *ensurePanel(const QString &id);
            void ensureContent(SVS::DockingPanel *panel, bool placeholder, std::function<void(bool)> completion);
            void floatRequest(SVS::DockingRequest *request);
            SVS::DockingHost *createFloatingHost(const QVariantMap &spec, bool deferRegistration = false);
            void restoreWindowGeometry(QQuickWindow *window, const QVariantMap &spec);
            void disposeFloatingHosts();
            ProjectWindowInterface *m_window;
            SVS::DockingContext *m_context;
            SVS::DockingHost *m_embeddedHost;
            WorkspacePanelLoader *m_loader;
            QHash<QString, QPointer<QQuickItem>> m_contents;
            QHash<QString, QList<std::function<void(bool)>>> m_pending;
            QHash<SVS::DockingHost *, QPointer<QQuickWindow>> m_floatingWindows;
            QVariantMap m_panelStates;
            ProjectWindowWorkspaceLayout m_pendingLayout;
            QString m_pendingShow;
            bool m_busy = false;
            bool m_ready = false;
            bool m_shuttingDown = false;
        };
    }
}

#endif

