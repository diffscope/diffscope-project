// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSCENARIO_P_H

#include <dspxproject/PickTrackColorScenario.h>

namespace DspxProject {

    class PickTrackColorScenarioPrivate {
        Q_DECLARE_PUBLIC(PickTrackColorScenario)
    public:
        PickTrackColorScenario *q_ptr;
        Document *const document;
        PickTrackColorStrategy *const strategy;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSCENARIO_P_H
