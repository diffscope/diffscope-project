// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_BATCHPROCESSADDON_H
#define DIFFSCOPE_BATCHPROCESS_BATCHPROCESSADDON_H

#include <QHash>
#include <QPointer>
#include <QString>
#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

#include <batchprocess/Script.h>

class QAbstractItemModel;

namespace Core {
    class ProjectWindowInterface;
    class WorkspacePanelRegistry;
}

namespace SVS {
    class DockingContext;
}

namespace BatchProcess::Internal {

    class ScriptPanel;

    class BatchProcessAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAbstractItemModel *actionModel READ actionModel CONSTANT)
    public:
        explicit BatchProcessAddOn(QObject *parent = nullptr);
        ~BatchProcessAddOn() override;

        QAbstractItemModel *actionModel() const;

        Q_INVOKABLE void executeAction(QObject *actionObject);
        Q_INVOKABLE void reloadScripts();

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        struct RegisteredScriptPanel {
            QPointer<ScriptPanel> wrapper;
            QPointer<QQmlComponent> component;
        };

        void refreshScriptPanels();
        void createScriptPanel(Script *script, qsizetype index, const Script::Panel &panel);
        void clearScriptPanels();

        QAbstractItemModel *m_actionModel{};
        QPointer<Core::ProjectWindowInterface> m_projectWindow;
        QPointer<Core::WorkspacePanelRegistry> m_panelRegistry;
        QPointer<SVS::DockingContext> m_dockingContext;
        QObject *m_scriptPanelContext{};
        QHash<QString, RegisteredScriptPanel> m_registeredScriptPanels;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_BATCHPROCESSADDON_H
