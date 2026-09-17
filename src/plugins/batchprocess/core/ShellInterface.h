// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SHELLINTERFACE_H
#define DIFFSCOPE_BATCHPROCESS_SHELLINTERFACE_H

#include <QJSValue>
#include <QObject>
#include <QScopedPointer>

#include <batchprocess/batchprocessglobal.h>

namespace Core {
    class ActionWindowInterfaceBase;
}

namespace BatchProcess {

    namespace Internal {
        class BatchProcessPlugin;
        class ShellModuleExtension;
    }

    class ScriptExecutionContext;
    class ShellInterfacePrivate;

    class BATCH_PROCESS_EXPORT ShellInterface : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ShellInterface)
    public:
        ~ShellInterface() override;

        static ShellInterface *instance();

        /** Wraps a live application window for the JavaScript engine associated with context. */
        QJSValue toJavaScriptValue(ScriptExecutionContext *context, Core::ActionWindowInterfaceBase *windowInterface);
        /** Resolves a Window value created by this interface for the JavaScript engine associated with context. */
        bool fromJavaScriptValue(ScriptExecutionContext *context, const QJSValue &value, Core::ActionWindowInterfaceBase **windowInterface) const;

    private:
        friend class Internal::BatchProcessPlugin;
        friend class Internal::ShellModuleExtension;

        explicit ShellInterface(QObject *parent = nullptr);

        QScopedPointer<ShellInterfacePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SHELLINTERFACE_H
