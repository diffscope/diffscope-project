// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACEPANELLOADER_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACEPANELLOADER_H

#include <QObject>
#include <QString>
#include <QVariant>

class QQmlComponent;
class QQuickItem;

namespace SVS {
    class DockingPanel;
}

namespace Core {
    namespace Internal {
        class WorkspacePanelLoader : public QObject {
            Q_OBJECT
        public:
            static QQuickItem *create(QQmlComponent *component, SVS::DockingPanel *panel, QString *error);
            static QQuickItem *createFallback(SVS::DockingPanel *panel, const QString &title, bool componentRegistered, const QString &error);
            static void loadState(QQuickItem *item, const QVariant &state);
            static QVariant saveState(QQuickItem *item, QString *error);
        };
    }
}

#endif

