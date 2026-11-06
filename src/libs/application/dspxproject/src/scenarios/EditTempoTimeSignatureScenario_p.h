// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESCENARIO_P_H

#include <dspxproject/EditTempoTimeSignatureScenario.h>

namespace DspxProject {

    class EditTempoTimeSignatureScenarioPrivate {
        Q_DECLARE_PUBLIC(EditTempoTimeSignatureScenario)
    public:
        EditTempoTimeSignatureScenario *q_ptr;
        Document *const document;
        EditTempoTimeSignatureStrategy *const strategy;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESCENARIO_P_H
