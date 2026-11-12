// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTPANELCONTEXT_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTPANELCONTEXT_H

#include <QJSValue>
#include <QObject>
#include <QPointer>
#include <QString>
#include <qqmlintegration.h>

namespace BatchProcess {
    class Script;
}

namespace BatchProcess::Internal {

    class ScriptPanel;

    class ScriptPanelContext : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
    public:
        explicit ScriptPanelContext(ScriptPanel *panel);
        void setScript(Script *script);

        Q_INVOKABLE QJSValue t(const QJSValue &key, const QJSValue &optionsOrDefaultValue = QJSValue(), const QJSValue &options = QJSValue());
        Q_INVOKABLE QJSValue invokeMethod(const QString &methodName, const QJSValue &arg = QJSValue());
        Q_INVOKABLE QJSValue invokeScriptMethod(const QString &scriptId, const QString &methodName, const QJSValue &arg = QJSValue());

    private:
        ScriptPanel *m_panel;
        QPointer<Script> m_script;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTPANELCONTEXT_H
