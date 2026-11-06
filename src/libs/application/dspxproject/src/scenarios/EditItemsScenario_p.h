// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITITEMSSCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_EDITITEMSSCENARIO_P_H

#include <dspxproject/EditItemsScenario.h>

namespace DspxProject {

    class EditItemsScenarioPrivate {
        Q_DECLARE_PUBLIC(EditItemsScenario)
    public:
        EditItemsScenario *q_ptr;
        Document *const document;
        EditItemsStrategy *const strategy;
        ParameterResamplingScenario *const resampling;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITITEMSSCENARIO_P_H
