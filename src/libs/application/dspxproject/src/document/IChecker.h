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

    class DSPXPROJECT_EXPORT IChecker : public QObject {
        Q_OBJECT
    public:
        explicit IChecker(QObject *parent = nullptr);
        ~IChecker() override;

        enum Level {
            Weak,
            Strong,
        };

        virtual QList<CheckWarning> runCheck(const opendspx::Model &model, Level level, bool failFast) = 0;
    };

    struct CheckWarning {
        IChecker::Level level;
        QString message;
        QString jsonPath;
        QString description;
        QList<QPair<QString, QVariant>> info;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_ICHECKER_H
