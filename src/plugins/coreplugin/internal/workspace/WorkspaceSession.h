// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACESESSION_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACESESSION_H

#include <functional>

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QVariantMap>

#include <coreplugin/internal/ProjectWindowWorkspaceLayout.h>

Q_MOC_INCLUDE(<QQuickItem>)
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
    class WorkspacePanelRegistry;

    namespace Internal {
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
            void apply(const ProjectWindowWorkspaceLayout &layout);
            ProjectWindowWorkspaceLayout snapshot(const QString &name) const;
            void shutdown();
        Q_SIGNALS:
            void busyChanged();
            void readyChanged();
            void shuttingDownChanged();
            void restored();
            void panelsChanged();
            void panelContentCreated(const QString &id, QQuickItem *item);
            void errorOccurred(const QString &message);

        private:
            struct PanelVersions {
                quint64 registration = 0;
                quint64 component = 0;
            };

            struct PanelContent {
                QPointer<QQuickItem> item;
                quint64 version = 0;
                bool fallback = false;
                bool statePending = false;
            };

            struct PanelRequest {
                quint64 registration;
                std::function<void(bool)> completion;
            };

            SVS::DockingPanel *ensurePanel(const QString &id);
            void schedulePanelUpdate(const QString &id);
            void schedulePanelUpdates();
            void updatePanels();
            void updatePanel(const QString &id);
            void unloadContent(SVS::DockingPanel *panel);
            void requestContent(SVS::DockingPanel *panel, std::function<void(bool)> completion);
            void finishPanelRequests(const QString &id, bool waiting);
            void finishLayoutRestore();
            void floatRequest(SVS::DockingRequest *request);
            QQuickWindow *floatingWindowForPanel(const SVS::DockingPanel *panel) const;
            SVS::DockingHost *createFloatingHost(const QVariantMap &spec, bool deferRegistration = false);
            void restoreWindowGeometry(QQuickWindow *window, const QVariantMap &spec);
            void disposeFloatingHosts();
            ProjectWindowInterface *m_window;
            QPointer<WorkspacePanelRegistry> m_registry;
            SVS::DockingContext *m_context;
            SVS::DockingHost *m_embeddedHost;
            QHash<QString, PanelVersions> m_panelVersions;
            QHash<QString, PanelContent> m_contents;
            QHash<QString, QList<PanelRequest>> m_panelRequests;
            QHash<QString, QString> m_panelTitles;
            QSet<QString> m_panelUpdates;
            QSet<QString> m_restoringPanelIds;
            QHash<SVS::DockingHost *, QPointer<QQuickWindow>> m_floatingWindows;
            QVariantMap m_panelFloatingGeometries;
            QVariantMap m_panelStates;
            ProjectWindowWorkspaceLayout m_restoringLayout;
            ProjectWindowWorkspaceLayout m_pendingLayout;
            bool m_panelUpdatePending = false;
            bool m_busy = false;
            bool m_ready = false;
            bool m_shuttingDown = false;
        };
    }
}

#endif

