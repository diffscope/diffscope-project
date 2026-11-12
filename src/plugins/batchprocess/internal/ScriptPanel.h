// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTPANEL_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTPANEL_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <qqmlintegration.h>

Q_MOC_INCLUDE(<QQmlComponent>)
Q_MOC_INCLUDE(<batchprocess/internal/ScriptPanelContext.h>)
Q_MOC_INCLUDE(<coreplugin/ProjectWindowInterface.h>)

class QQmlComponent;
class QUrl;

namespace Core {
    class ProjectWindowInterface;
}

namespace BatchProcess {
    class Script;
}

namespace BatchProcess::Internal {

    class ScriptPanelContext;

    class ScriptPanel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_CLASSINFO("DefaultProperty", "panel")
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle WRITE setWindowHandle NOTIFY windowHandleChanged REQUIRED)
        Q_PROPERTY(QQmlComponent *panel READ panel WRITE setPanel NOTIFY panelChanged)
        Q_PROPERTY(ScriptPanelContext *scriptContext READ scriptContext CONSTANT)
    public:
        explicit ScriptPanel(QObject *parent = nullptr);

        Core::ProjectWindowInterface *windowHandle() const;
        void setWindowHandle(Core::ProjectWindowInterface *windowHandle);

        QQmlComponent *panel() const;
        void setPanel(QQmlComponent *panel);

        ScriptPanelContext *scriptContext() const;
        static QObject *create(QQmlComponent *component, Script *script, Core::ProjectWindowInterface *window, QString *error);
        static void reportError(const QString &scriptId, const QString &name, const QUrl &source, const QString &details, Core::ProjectWindowInterface *window = nullptr);

    Q_SIGNALS:
        void windowHandleChanged();
        void panelChanged();

    private:
        QPointer<Core::ProjectWindowInterface> m_windowHandle;
        QPointer<QQmlComponent> m_panel;
        ScriptPanelContext *m_scriptContext;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTPANEL_H
