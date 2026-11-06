// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITLOOPSCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_EDITLOOPSCENARIO_P_H

#include <dspxproject/EditLoopScenario.h>

namespace DspxProject {

    class EditLoopScenarioPrivate {
        Q_DECLARE_PUBLIC(EditLoopScenario)
    public:
        EditLoopScenario *q_ptr;
        Document *const document;
        EditLoopStrategy *const strategy;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITLOOPSCENARIO_P_H
