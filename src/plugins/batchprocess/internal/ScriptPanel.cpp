// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ScriptPanel.h"

#include <QQmlComponent>
#include <QQmlError>
#include <QQmlIncubator>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/JavaScriptConsoleInterface.h>
#include <batchprocess/internal/ScriptPanelContext.h>

namespace BatchProcess::Internal {

    namespace {

        class ScriptPanelIncubator : public QQmlIncubator {
        public:
            explicit ScriptPanelIncubator(Script *script) : QQmlIncubator(Synchronous), m_script(script) {
            }

        protected:
            void setInitialState(QObject *object) override {
                if (const auto panel = qobject_cast<ScriptPanel *>(object)) {
                    panel->scriptContext()->setScript(m_script);
                }
            }

        private:
            Script *m_script;
        };

    }

    ScriptPanel::ScriptPanel(QObject *parent) : QObject(parent), m_scriptContext(new ScriptPanelContext(this)) {
    }

    Core::ProjectWindowInterface *ScriptPanel::windowHandle() const {
        return m_windowHandle;
    }

    void ScriptPanel::setWindowHandle(Core::ProjectWindowInterface *windowHandle) {
        if (m_windowHandle.data() == windowHandle) {
            return;
        }
        m_windowHandle = windowHandle;
        Q_EMIT windowHandleChanged();
    }

    QQmlComponent *ScriptPanel::panel() const {
        return m_panel;
    }

    void ScriptPanel::setPanel(QQmlComponent *panel) {
        if (m_panel.data() == panel) {
            return;
        }
        m_panel = panel;
        Q_EMIT panelChanged();
    }

    ScriptPanelContext *ScriptPanel::scriptContext() const {
        return m_scriptContext;
    }

    QObject *ScriptPanel::create(QQmlComponent *component, Script *script, Core::ProjectWindowInterface *window, QString *error) {
        ScriptPanelIncubator incubator(script);
        incubator.setInitialProperties({{"windowHandle", QVariant::fromValue(window)}});
        component->create(incubator, component->creationContext());
        if (const auto object = incubator.object()) {
            return object;
        }

        QStringList errors;
        for (const auto &qmlError : incubator.errors()) {
            errors.append(qmlError.toString());
        }
        *error = errors.isEmpty() ? component->errorString().trimmed() : errors.join(QLatin1Char('\n'));
        return nullptr;
    }

    void ScriptPanel::reportError(const QString &scriptId, const QString &name, const QUrl &source, const QString &details, Core::ProjectWindowInterface *window) {
        const auto title = BatchProcessInterface::tr("Batch Process");
        const auto reason = details.isEmpty() ? BatchProcessInterface::tr("The QML component could not be loaded or created.") : details;
        const auto message = BatchProcessInterface::tr("Failed to load panel %1 from script %2:\n\n%3").arg(name, scriptId, reason);
        const QPointer windowGuard(window);
        if (const auto console = JavaScriptConsoleInterface::instance()) {
            console->appendMessage(JavaScriptConsoleInterface::Error, message, source);
        }
        if (windowGuard) {
            windowGuard->sendNotification(SVS::SVSCraft::Critical, title, message);
        } else if (!window) {
            Core::CoreInterface::sendNotification(SVS::SVSCraft::Critical, title, message);
        }
    }

}

#include "moc_ScriptPanel.cpp"
