// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESCENARIO_P_H

#include <dspxproject/EditKeySignatureScenario.h>

namespace DspxProject {

    class EditKeySignatureScenarioPrivate {
        Q_DECLARE_PUBLIC(EditKeySignatureScenario)
    public:
        EditKeySignatureScenario *q_ptr;
        Document *const document;
        EditKeySignatureStrategy *const strategy;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESCENARIO_P_H
