// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_FILLLYRICSSCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_FILLLYRICSSCENARIO_P_H

#include <dspxproject/FillLyricsScenario.h>

namespace DspxProject {

    class FillLyricsScenarioPrivate {
        Q_DECLARE_PUBLIC(FillLyricsScenario)
    public:
        FillLyricsScenario *q_ptr;
        Document *const document;
        FillLyricsStrategy *const strategy;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_FILLLYRICSSCENARIO_P_H
