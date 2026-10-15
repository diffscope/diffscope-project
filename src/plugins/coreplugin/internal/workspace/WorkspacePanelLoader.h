// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACEPANELLOADER_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACEPANELLOADER_H

#include <functional>

#include <QObject>
#include <QPointer>
#include <QVariant>

class QQmlComponent;
class QQuickItem;

namespace SVS {
    class DockingPanel;
}

namespace Core {
    class ProjectWindowInterface;

    namespace Internal {
        class WorkspacePanelLoader : public QObject {
            Q_OBJECT
        public:
            using Completion = std::function<void(QQuickItem *, const QString &)>;
            explicit WorkspacePanelLoader(ProjectWindowInterface *window, QObject *parent = nullptr);
            void load(SVS::DockingPanel *panel, bool placeholder, Completion completion);
            void cancel();
            static bool validateState(QQuickItem *item, const QVariant &state, QString *error);
            static bool prepareState(QQuickItem *item, const QVariant &state, QString *error);
            static bool applyState(QQuickItem *item, const QVariant &state, QString *error);
            static bool finishState(QQuickItem *item, QString *error);
            static bool abortState(QQuickItem *item, QString *error);
            static bool loadState(QQuickItem *item, const QVariant &state, QString *error);
            static QVariant saveState(QQuickItem *item, QString *error);

        private:
            static bool invokeStateMethod(QQuickItem *item, const QString &method, const QVariant &state, bool withState, QString *error);
            void create(QQmlComponent *component, SVS::DockingPanel *panel, bool placeholder, Completion completion);
            ProjectWindowInterface *m_window;
            bool m_cancelled = false;
        };
    }
}

#endif

