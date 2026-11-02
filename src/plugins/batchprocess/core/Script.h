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
    }

    class ScriptAction;
    class ScriptMethod;
    class ScriptPrivate;

    class BATCH_PROCESS_EXPORT Script : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Script)
    public:
        ~Script() override;

        /** Returns whether the script is registered as a built-in script. */
        bool isBuiltin() const;
        /** Returns the physical entry file or package directory, or an empty string for a script registered with a factory. */
        QString filePath() const;
        /** Returns the sandbox root directory used for module and file-system path resolution, or an empty string for a script registered with a factory. */
        QString rootPath() const;
        ScriptMetadata metadata() const;
        QList<ScriptAction *> actions() const;
        QList<ScriptMethod *> methods() const;

    private:
        friend class Internal::BatchProcessRuntime;
        explicit Script(const QString &filePath, const QString &rootPath, bool builtin, QObject *parent = nullptr);

        QScopedPointer<ScriptPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPT_H
