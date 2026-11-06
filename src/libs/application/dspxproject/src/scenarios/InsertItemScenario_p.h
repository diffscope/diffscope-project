// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_INSERTITEMSCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_INSERTITEMSCENARIO_P_H

#include <dspxproject/InsertItemScenario.h>

namespace DspxProject {

    class InsertItemScenarioPrivate {
        Q_DECLARE_PUBLIC(InsertItemScenario)
    public:
        InsertItemScenario *q_ptr;
        Document *const document;
        InsertItemStrategy *const strategy;
        const InsertItemScenario::Options options;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_INSERTITEMSCENARIO_P_H
