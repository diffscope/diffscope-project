// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CHECKERREGISTRY_P_H
#define DIFFSCOPE_DSPXPROJECT_CHECKERREGISTRY_P_H

#include <dspxproject/CheckerRegistry.h>

namespace DspxProject {
    class CheckerRegistryPrivate {
        Q_DECLARE_PUBLIC(CheckerRegistry)
    public:
        CheckerRegistry *q_ptr;
        QList<IChecker *> checkers;
    };
}

#endif // DIFFSCOPE_DSPXPROJECT_CHECKERREGISTRY_P_H
