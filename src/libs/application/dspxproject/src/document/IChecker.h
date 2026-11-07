// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_ICHECKER_H
#define DIFFSCOPE_DSPXPROJECT_ICHECKER_H

#include <QObject>
#include <QVariant>

#include <dspxproject/dspxprojectglobal.h>

namespace opendspx {
    struct Model;
}

namespace DspxProject {

    struct CheckWarning;

    /** Defines serialized project validation without modifying the model. */
    class DSPXPROJECT_EXPORT IChecker : public QObject {
        Q_OBJECT
    public:
        /** Creates a checker with an optional QObject parent. */
        explicit IChecker(QObject *parent = nullptr);
        /** Destroys the checker. */
        ~IChecker() override;

        /** The validation strength requested by the caller. */
        enum Level {
            Weak, /**< Basic validation. */
            Strong, /**< Complete validation. */
        };

        /**
         * Validates model at the requested level and returns detected warnings.
         * Returns an empty list if no warnings are reported. If failFast is true,
         * validation may stop after the first detected warning.
         */
        virtual QList<CheckWarning> runCheck(const opendspx::Model &model, Level level, bool failFast) = 0;
    };

    /** A validation warning and its location and diagnostic details. */
    struct CheckWarning {
        /** The validation level associated with the warning. */
        IChecker::Level level;
        /** The user-visible warning summary. */
        QString message;
        /** The affected JSON path, empty if no specific location applies. */
        QString jsonPath;
        /** The additional user-visible explanation, empty if absent. */
        QString description;
        /** The diagnostic names and associated values. */
        QList<QPair<QString, QVariant>> info;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_ICHECKER_H
