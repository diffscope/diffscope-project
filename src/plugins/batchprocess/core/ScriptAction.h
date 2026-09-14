// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTACTION_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTACTION_H

#include <QObject>
#include <QString>

#include <batchprocess/batchprocessglobal.h>

class QJSValue;

namespace BatchProcess {

    namespace Internal {
        class BatchProcessRuntime;
    }

    class Script;
    class ScriptActionPrivate;

    class BATCH_PROCESS_EXPORT ScriptAction : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ScriptAction)
    public:
        ~ScriptAction() override;

        Script *script() const;
        QString name() const;
        QString description() const;
        bool requiresProject() const;
        /** Valid only until the next successful script reload. Prefer BatchProcessInterface::executeAction(). */
        QJSValue executeFunction() const;

    private:
        friend class Internal::BatchProcessRuntime;
        explicit ScriptAction(Script *script, QObject *parent = nullptr);

        QScopedPointer<ScriptActionPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTACTION_H
