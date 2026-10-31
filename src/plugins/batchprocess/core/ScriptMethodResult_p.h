// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTMETHODRESULT_P_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTMETHODRESULT_P_H

#include <batchprocess/ScriptMethodResult.h>

#include <QSharedData>

namespace BatchProcess {

    class ScriptMethodResultData : public QSharedData {
    public:
        ScriptMethodResult::Status status{ScriptMethodResult::Success};
        QJsonValue value{QJsonValue::Undefined};
        QString errorName;
        QString errorMessage;
        QJsonValue errorData{QJsonValue::Undefined};
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTMETHODRESULT_P_H
