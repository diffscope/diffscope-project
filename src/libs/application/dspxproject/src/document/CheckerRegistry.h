// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CHECKERREGISTRY_H
#define DIFFSCOPE_DSPXPROJECT_CHECKERREGISTRY_H

#include <QObject>

#include <dspxproject/IChecker.h>

namespace opendspx {
    struct Model;
}

namespace DspxProject {

    class CheckerRegistryPrivate;

    class DSPXPROJECT_EXPORT CheckerRegistry : public IChecker {
        Q_OBJECT
        Q_DECLARE_PRIVATE(CheckerRegistry)
    public:
        explicit CheckerRegistry(QObject *parent = nullptr);
        ~CheckerRegistry() override;

        void registerChecker(IChecker *checker);
        QList<IChecker *> checkers() const;

        QList<CheckWarning> runCheck(const opendspx::Model &model, Level level, bool failFast) override;

    private:
        QScopedPointer<CheckerRegistryPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CHECKERREGISTRY_H
