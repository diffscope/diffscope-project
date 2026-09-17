// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ShellInterface.h"
#include "ShellInterface_p.h"

#include <QLoggingCategory>

#include <coreplugin/ActionWindowInterfaceBase.h>

#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/internal/ShellModuleExtension.h>

namespace BatchProcess {

    Q_STATIC_LOGGING_CATEGORY(lcShellInterface, "diffscope.batchprocess.shell.interface")

    namespace {

        ShellInterface *s_instance{};

        bool isUsableWindow(Core::ActionWindowInterfaceBase *windowInterface) {
            return windowInterface && !windowInterface->isEffectivelyClosed() && windowInterface->window();
        }

    }

    ShellInterfacePrivate::ShellInterfacePrivate(ShellInterface *q) : q_ptr(q) {
    }

    ShellInterface::ShellInterface(QObject *parent) : QObject(parent), d_ptr(new ShellInterfacePrivate(this)) {
        Q_ASSERT(!s_instance);
        s_instance = this;
    }

    ShellInterface::~ShellInterface() {
        s_instance = nullptr;
    }

    ShellInterface *ShellInterface::instance() {
        return s_instance;
    }

    QJSValue ShellInterface::toJavaScriptValue(ScriptExecutionContext *context, Core::ActionWindowInterfaceBase *windowInterface) {
        Q_D(ShellInterface);
        if (!context || !context->engine() || !isUsableWindow(windowInterface) || !d->moduleExtension) {
            qCWarning(lcShellInterface) << "Unable to wrap an application window for JavaScript";
            return QJSValue(QJSValue::UndefinedValue);
        }
        return d->moduleExtension->wrapWindow(context, windowInterface);
    }

    bool ShellInterface::fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, Core::ActionWindowInterfaceBase **windowInterface) const {
        Q_D(const ShellInterface);
        if (!windowInterface) {
            return false;
        }
        *windowInterface = nullptr;
        if (!context || !context->engine() || !d->moduleExtension) {
            qCWarning(lcShellInterface) << "Unable to resolve a JavaScript application window";
            return false;
        }
        return d->moduleExtension->unwrapWindow(context, value, windowInterface);
    }

}

#include "moc_ShellInterface.cpp"
