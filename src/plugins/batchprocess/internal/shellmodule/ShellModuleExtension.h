// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SHELLMODULEEXTENSION_H
#define DIFFSCOPE_BATCHPROCESS_SHELLMODULEEXTENSION_H

#include <QHash>
#include <QJSValue>
#include <QObject>
#include <QString>

class QJSEngine;

namespace Core {
    class ActionWindowInterfaceBase;
}

namespace BatchProcess {

    class BatchProcessInterface;
    class FileSystemAccessInterface;
    class ScriptExecutionContext;
    class ShellInterface;

}

namespace BatchProcess::Internal {

    class ShellModuleBridge;
    class WindowBridge;

    class ShellModuleExtension : public QObject {
        Q_OBJECT
    public:
        explicit ShellModuleExtension(BatchProcessInterface *batchProcessInterface, ShellInterface *shellInterface, FileSystemAccessInterface *fileSystemAccessInterface, QObject *parent = nullptr);
        ~ShellModuleExtension() override;

        QJSValue wrapWindow(ScriptExecutionContext *context, Core::ActionWindowInterfaceBase *windowInterface) const;
        bool unwrapWindow(ScriptExecutionContext *context, const QJSValue &value, Core::ActionWindowInterfaceBase **windowInterface) const;

    private:
        friend class ShellModuleBridge;
        friend class WindowBridge;

        void installIntoEngine(QJSEngine *engine, ScriptExecutionContext *context);
        QJSValue createModule(ScriptExecutionContext *context) const;
        QJSValue helper(ScriptExecutionContext *context, const QString &name) const;

        BatchProcessInterface *m_batchProcessInterface;
        ShellInterface *m_shellInterface;
        FileSystemAccessInterface *m_fileSystemAccessInterface;
        QHash<QJSEngine *, QJSValue> m_engineBundles;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SHELLMODULEEXTENSION_H
