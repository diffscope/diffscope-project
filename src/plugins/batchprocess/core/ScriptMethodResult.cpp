// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ScriptMethodResult.h"
#include "ScriptMethodResult_p.h"

namespace BatchProcess {

    ScriptMethodResult::ScriptMethodResult() = default;
    ScriptMethodResult::ScriptMethodResult(const ScriptMethodResult &other) = default;
    ScriptMethodResult::ScriptMethodResult(ScriptMethodResult &&other) noexcept = default;
    ScriptMethodResult::~ScriptMethodResult() = default;

    ScriptMethodResult &ScriptMethodResult::operator=(const ScriptMethodResult &other) = default;
    ScriptMethodResult &ScriptMethodResult::operator=(ScriptMethodResult &&other) noexcept = default;

    ScriptMethodResult ScriptMethodResult::success(const QJsonValue &value) {
        ScriptMethodResult result;
        result.d = new ScriptMethodResultData;
        result.d->value = value;
        return result;
    }

    ScriptMethodResult ScriptMethodResult::logicError(const QString &name, const QString &message, const QJsonValue &data) {
        ScriptMethodResult result;
        result.d = new ScriptMethodResultData;
        result.d->status = LogicError;
        result.d->errorName = name;
        result.d->errorMessage = message;
        result.d->errorData = data;
        return result;
    }

    ScriptMethodResult ScriptMethodResult::internalError(const QString &name, const QString &message) {
        ScriptMethodResult result;
        result.d = new ScriptMethodResultData;
        result.d->status = InternalError;
        result.d->errorName = name;
        result.d->errorMessage = message;
        return result;
    }

    ScriptMethodResult::Status ScriptMethodResult::status() const {
        return d ? d->status : Success;
    }

    QJsonValue ScriptMethodResult::value() const {
        return d ? d->value : QJsonValue(QJsonValue::Undefined);
    }

    QString ScriptMethodResult::errorName() const {
        return d ? d->errorName : QString();
    }

    QString ScriptMethodResult::errorMessage() const {
        return d ? d->errorMessage : QString();
    }

    QJsonValue ScriptMethodResult::errorData() const {
        return d ? d->errorData : QJsonValue(QJsonValue::Undefined);
    }

}
