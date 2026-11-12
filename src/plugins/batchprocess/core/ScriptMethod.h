// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTMETHOD_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTMETHOD_H

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QString>

#include <batchprocess/batchprocessglobal.h>

class QJSValue;

namespace BatchProcess {

    namespace Internal {
        class BatchProcessRuntime;
    }

    class Script;
    class ScriptMethodPrivate;

    class BATCH_PROCESS_EXPORT ScriptMethod : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ScriptMethod)
        Q_PROPERTY(bool internal READ isInternal CONSTANT)
    public:
        ~ScriptMethod() override;

        Script *script() const;
        QString name() const;
        QString summary() const;
        QString description() const;
        bool isInternal() const;
        QJsonArray params() const;
        /** Returns the result descriptor, or an undefined value if omitted. */
        QJsonValue result() const;
        /** Returns annotations with all default values applied. */
        QJsonObject annotations() const;
        /** Returns detached metadata without the execution function. Schemas are not evaluated. */
        QJsonObject metadata() const;
        /** Returns the handler, valid until the next successful reload. Prefer BatchProcessInterface::executeMethod(). */
        QJSValue executeFunction() const;

    private:
        friend class Internal::BatchProcessRuntime;
        explicit ScriptMethod(Script *script, QObject *parent = nullptr);

        QScopedPointer<ScriptMethodPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTMETHOD_H
