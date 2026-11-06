// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CheckerRegistry.h"
#include "CheckerRegistry_p.h"


namespace DspxProject {

    CheckerRegistry::CheckerRegistry(QObject *parent) : IChecker(parent), d_ptr(new CheckerRegistryPrivate) {
        Q_D(CheckerRegistry);
        d->q_ptr = this;
    }

    CheckerRegistry::~CheckerRegistry() = default;

    void CheckerRegistry::registerChecker(IChecker *checker) {
        Q_D(CheckerRegistry);
        d->checkers.append(checker);
    }

    QList<IChecker *> CheckerRegistry::checkers() const {
        Q_D(const CheckerRegistry);
        return d->checkers;
    }

    QList<CheckWarning> CheckerRegistry::runCheck(const opendspx::Model &model, Level level, bool failFast) {
        Q_D(CheckerRegistry);
        for (auto checker : d->checkers) {
            auto warnings = checker->runCheck(model, level, failFast);
            if (failFast && !warnings.isEmpty())
                return warnings;
        }
        return {};
    }

}
