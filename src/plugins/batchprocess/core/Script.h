// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPT_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPT_H

#include <QList>
#include <QObject>

#include <batchprocess/ScriptMetadata.h>
#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    namespace Internal {
        class BatchProcessRuntime;
        class RuntimeModuleExtension;
    }

    class ScriptAction;
    class FileSystemAccessInterface;
    class FileSystemAccessInterfacePrivate;
    class ScriptPrivate;

    class BATCH_PROCESS_EXPORT Script : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Script)
    public:
        ~Script() override;

        /** Empty for a built-in script; otherwise the physical entry file or package directory. */
        QString filePath() const;
        ScriptMetadata metadata() const;
        QList<ScriptAction *> actions() const;

    private:
        friend class Internal::BatchProcessRuntime;
        friend class Internal::RuntimeModuleExtension;
        friend class FileSystemAccessInterface;
        friend class FileSystemAccessInterfacePrivate;
        explicit Script(const QString &filePath, const QString &rootPath, QObject *parent = nullptr);

        QScopedPointer<ScriptPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPT_H
