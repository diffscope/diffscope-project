// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_PARAMETEREXPRESSIONUTILS_P_H
#define DIFFSCOPE_SYNTH_INTERNAL_PARAMETEREXPRESSIONUTILS_P_H

#include <QString>

struct te_expr;

namespace Synth::Internal::ParameterExpressionUtils {

    te_expr *compile(const QString &expression, double *variable, int *errorPosition = nullptr);
    bool evaluate(const QString &expression, double input, double *result,
                  int *errorPosition = nullptr);
    bool validateDisplayTemplate(const QString &displayTemplate, int *errorPosition = nullptr);
    QString formatDisplayValue(const QString &displayTemplate, double value);

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_PARAMETEREXPRESSIONUTILS_P_H
