// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTMETHODRESULT_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTMETHODRESULT_H

#include <QJsonValue>
#include <QSharedDataPointer>
#include <QString>

#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    class ScriptMethodResultData;

    class BATCH_PROCESS_EXPORT ScriptMethodResult {
    public:
        enum Status {
            Success,
            LogicError,
            InternalError,
        };

        /** Creates a successful result with an undefined value. */
        ScriptMethodResult();
        ScriptMethodResult(const ScriptMethodResult &other);
        ScriptMethodResult(ScriptMethodResult &&other) noexcept;
        ~ScriptMethodResult();

        ScriptMethodResult &operator=(const ScriptMethodResult &other);
        ScriptMethodResult &operator=(ScriptMethodResult &&other) noexcept;

        static ScriptMethodResult success(const QJsonValue &value = QJsonValue(QJsonValue::Undefined));
        static ScriptMethodResult logicError(const QString &name, const QString &message, const QJsonValue &data = QJsonValue(QJsonValue::Undefined));
        static ScriptMethodResult internalError(const QString &name, const QString &message);

        Status status() const;
        /** Returns the successful execution value, or an undefined value if absent or for errors. */
        QJsonValue value() const;
        /** Returns the exception name, or an empty string on success or if the thrown value has no name. */
        QString errorName() const;
        /** Returns the exception message, or an empty string on success. */
        QString errorMessage() const;
        /** Returns the logic error data, or an undefined value if absent, not serializable as JSON, or for other statuses. */
        QJsonValue errorData() const;

    private:
        QSharedDataPointer<ScriptMethodResultData> d;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTMETHODRESULT_H
