// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ScriptPanelContext.h"

#include <algorithm>

#include <QJSEngine>
#include <QJSManagedValue>
#include <QJsonObject>
#include <QJsonValue>
#include <QQmlEngine>

#include <coreplugin/ProjectWindowInterface.h>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/Script.h>
#include <batchprocess/ScriptMethod.h>
#include <batchprocess/ScriptMethodResult.h>
#include <batchprocess/internal/BatchProcessRuntime.h>
#include <batchprocess/internal/ScriptPanel.h>
#include <batchprocess/private/BatchProcessInterface_p.h>

namespace BatchProcess::Internal {

    namespace {

        QJsonValue serializeArgument(QJSEngine *engine, const QJSValue &argument) {
            if (argument.isUndefined()) {
                return QJsonValue(QJsonValue::Undefined);
            }
            const QJSManagedValue stringify(engine->globalObject().property(QStringLiteral("JSON")).property(QStringLiteral("stringify")), engine);
            const auto json = stringify.call({argument});
            return json.isString() ? QJsonValue::fromJson(json.toString().toUtf8()) : QJsonValue(QJsonValue::Undefined);
        }

        QJSValue throwCallError(QJSEngine *engine, const QString &message, const QJSValue &cause) {
            auto error = engine->newErrorObject(QJSValue::GenericError, message);
            error.setProperty(QStringLiteral("cause"), cause);
            engine->throwError(error);
            return {};
        }

        QJSValue jsonToScriptValue(QJSEngine *engine, const QJsonValue &value) {
            if (value.isUndefined()) {
                return {};
            }
            return value.isNull() ? QJSValue(QJSValue::NullValue) : engine->toScriptValue(value.toVariant());
        }

        QJSValue completeCall(QJSEngine *engine, const QString &message, const ScriptMethodResult &result) {
            if (result.status() == ScriptMethodResult::Success) {
                return jsonToScriptValue(engine, result.value());
            }
            auto cause = engine->newErrorObject(QJSValue::GenericError, result.errorMessage());
            cause.setProperty(QStringLiteral("name"), result.errorName());
            if (result.status() == ScriptMethodResult::LogicError) {
                cause.setProperty(QStringLiteral("logicError"), true);
                const auto data = result.errorData();
                if (!data.isUndefined()) {
                    cause.setProperty(QStringLiteral("data"), jsonToScriptValue(engine, data));
                }
            }
            return throwCallError(engine, message + QLatin1Char('\n') + result.errorMessage(), cause);
        }

        ScriptMethodResult inactivePanelError() {
            return ScriptMethodResult::internalError(QStringLiteral("ReferenceError"), BatchProcessInterface::tr("The panel script is no longer active."));
        }

    }

    ScriptPanelContext::ScriptPanelContext(ScriptPanel *panel) : QObject(panel), m_panel(panel) {
    }

    void ScriptPanelContext::setScript(Script *script) {
        m_script = script;
    }

    QJSValue ScriptPanelContext::t(const QJSValue &key, const QJSValue &optionsOrDefaultValue, const QJSValue &options) {
        auto engine = qmlEngine(m_panel);
        if (!engine) {
            return {};
        }
        const auto script = m_script;
        const auto message = BatchProcessInterface::tr("Failed to translate script panel text.");
        if (!script) {
            return completeCall(engine, message, inactivePanelError());
        }

        QList<QJsonValue> arguments;
        arguments.reserve(3);
        for (const auto &argument : {key, optionsOrDefaultValue, options}) {
            const auto value = serializeArgument(engine, argument);
            if (engine->hasError()) {
                return throwCallError(engine, message, engine->catchError());
            }
            if (!argument.isUndefined() && value.isUndefined()) {
                return completeCall(engine, message, ScriptMethodResult::internalError(QStringLiteral("TypeError"), BatchProcessInterface::tr("Translation arguments must be serializable as JSON.")));
            }
            arguments.append(value);
        }
        while (arguments.size() > 1 && arguments.constLast().isUndefined()) {
            arguments.removeLast();
        }

        const auto batchProcess = BatchProcessInterface::instance();
        if (!script || !batchProcess || !batchProcess->d_func()->runtime) {
            return completeCall(engine, message, inactivePanelError());
        }
        return completeCall(engine, message, batchProcess->d_func()->runtime->translate(script.data(), arguments));
    }

    QJSValue ScriptPanelContext::invokeMethod(const QString &methodName, const QJSValue &arg) {
        return invokeScriptMethod(m_script ? m_script->metadata().id() : QString(), methodName, arg);
    }

    QJSValue ScriptPanelContext::invokeScriptMethod(const QString &scriptId, const QString &methodName, const QJSValue &arg) {
        auto engine = qmlEngine(m_panel);
        if (!engine) {
            return {};
        }
        const auto caller = m_script;
        const QPointer window(m_panel->windowHandle());
        const auto message = BatchProcessInterface::tr("Failed to invoke method %1 in script %2.").arg(methodName, scriptId);
        if (!caller) {
            return completeCall(engine, message, inactivePanelError());
        }

        QJsonObject params;
        if (!arg.isUndefined()) {
            const auto value = serializeArgument(engine, arg);
            if (engine->hasError()) {
                return throwCallError(engine, message, engine->catchError());
            }
            if (!value.isObject()) {
                return completeCall(engine, message, ScriptMethodResult::internalError(QStringLiteral("TypeError"), BatchProcessInterface::tr("Method arguments must be a JSON object.")));
            }
            params = value.toObject();
        }

        const auto batchProcess = BatchProcessInterface::instance();
        if (!caller || !batchProcess) {
            return completeCall(engine, message, inactivePanelError());
        }
        if (!window || window->isEffectivelyClosed()) {
            return completeCall(engine, message, ScriptMethodResult::internalError(QStringLiteral("ReferenceError"), BatchProcessInterface::tr("The panel window is no longer active.")));
        }
        const auto scripts = batchProcess->scripts();
        if (!scripts.contains(caller.data())) {
            return completeCall(engine, message, inactivePanelError());
        }
        const auto script = std::ranges::find_if(scripts, [&scriptId](const Script *item) {
            return item->metadata().id() == scriptId;
        });
        if (script == scripts.cend()) {
            return completeCall(engine, message, ScriptMethodResult::internalError(QStringLiteral("ReferenceError"), BatchProcessInterface::tr("The target script is not loaded.")));
        }
        const auto methods = (*script)->methods();
        const auto method = std::ranges::find_if(methods, [&methodName](const ScriptMethod *item) {
            return item->name() == methodName;
        });
        if (method == methods.cend()) {
            return completeCall(engine, message, ScriptMethodResult::internalError(QStringLiteral("ReferenceError"), BatchProcessInterface::tr("The target script does not define this method.")));
        }
        return completeCall(engine, message, batchProcess->executeMethod(*method, params, window.data()));
    }

}

#include "moc_ScriptPanelContext.cpp"
