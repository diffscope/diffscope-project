// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ParameterExpressionUtils_p.h"

#include <tinyexpr.h>

#include <cmath>
#include <limits>

#include <QLocale>

namespace {

    double sign(double value) {
        return static_cast<double>((value > 0.0) - (value < 0.0));
    }

    bool parseFixedPointSpecifier(const QString &displayTemplate, qsizetype index,
                                  qsizetype *specifierLength, int *precision = nullptr) {
        if (index + 2 >= displayTemplate.size() || displayTemplate.at(index) != u'%'
            || displayTemplate.at(index + 1) != u'.') {
            return false;
        }

        auto cursor = index + 2;
        int parsedPrecision{};
        bool hasDigit{};
        while (cursor < displayTemplate.size()) {
            const auto character = displayTemplate.at(cursor);
            if (character < u'0' || character > u'9')
                break;

            const auto digit = character.unicode() - u'0';
            if (parsedPrecision > (std::numeric_limits<int>::max() - digit) / 10)
                return false;
            parsedPrecision = parsedPrecision * 10 + digit;
            hasDigit = true;
            ++cursor;
        }

        if (!hasDigit || cursor >= displayTemplate.size() || displayTemplate.at(cursor) != u'f')
            return false;

        if (specifierLength)
            *specifierLength = cursor - index + 1;
        if (precision)
            *precision = parsedPrecision;
        return true;
    }

}

namespace Synth::Internal::ParameterExpressionUtils {

    te_expr *compile(const QString &expression, double *variable, int *errorPosition) {
        for (qsizetype index = 0; index < expression.size(); ++index) {
            if (expression.at(index).category() == QChar::Other_Control) {
                if (errorPosition)
                    *errorPosition = static_cast<int>(index + 1);
                return nullptr;
            }
        }
        const auto utf8 = expression.toUtf8();
        const te_variable variables[] = {
            {"sign", reinterpret_cast<const void *>(sign), TE_FUNCTION1 | TE_FLAG_PURE, nullptr},
            {"x", variable, TE_VARIABLE, nullptr},
        };
        int localError{};
        auto result = te_compile(utf8.constData(), variables, 2, &localError);
        if (errorPosition)
            *errorPosition = localError;
        return result;
    }

    bool evaluate(const QString &expression, double input, double *result, int *errorPosition) {
        double variable = input;
        auto compiled = compile(expression, &variable, errorPosition);
        if (!compiled)
            return false;
        const auto value = te_eval(compiled);
        te_free(compiled);
        if (!std::isfinite(value))
            return false;
        if (result)
            *result = value;
        return true;
    }

    bool validateDisplayTemplate(const QString &displayTemplate, int *errorPosition) {
        for (qsizetype index = 0; index < displayTemplate.size();) {
            if (displayTemplate.at(index) != u'%') {
                ++index;
                continue;
            }
            if (index + 1 < displayTemplate.size() &&
                (displayTemplate.at(index + 1) == u'%' || displayTemplate.at(index + 1) == u'd')) {
                index += 2;
                continue;
            }
            qsizetype specifierLength{};
            if (parseFixedPointSpecifier(displayTemplate, index, &specifierLength)) {
                index += specifierLength;
                continue;
            }
            if (errorPosition)
                *errorPosition = static_cast<int>(index + 1);
            return false;
        }
        if (errorPosition)
            *errorPosition = 0;
        return true;
    }

    QString formatDisplayValue(const QString &displayTemplate, double value) {
        QString result;
        result.reserve(displayTemplate.size() + 16);
        const QLocale locale;
        for (qsizetype index = 0; index < displayTemplate.size();) {
            if (displayTemplate.at(index) != u'%') {
                result.append(displayTemplate.at(index++));
                continue;
            }
            if (index + 1 < displayTemplate.size() && displayTemplate.at(index + 1) == u'%') {
                result.append(u'%');
                index += 2;
            } else if (index + 1 < displayTemplate.size() && displayTemplate.at(index + 1) == u'd') {
                qint64 integerValue{};
                if (value >= static_cast<double>(std::numeric_limits<qint64>::max()))
                    integerValue = std::numeric_limits<qint64>::max();
                else if (value <= static_cast<double>(std::numeric_limits<qint64>::min()))
                    integerValue = std::numeric_limits<qint64>::min();
                else
                    integerValue = static_cast<qint64>(std::round(value));
                result.append(locale.toString(integerValue));
                index += 2;
            } else {
                qsizetype specifierLength{};
                int precision{};
                if (parseFixedPointSpecifier(displayTemplate, index, &specifierLength, &precision)) {
                    result.append(locale.toString(value, 'f', precision));
                    index += specifierLength;
                    continue;
                }

                // Configurations are validated before reaching runtime. Keeping an inert literal
                // fallback here prevents an untrusted template from becoming a format string.
                result.append(u'%');
                ++index;
            }
        }
        return result;
    }

}
