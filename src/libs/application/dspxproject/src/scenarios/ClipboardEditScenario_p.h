// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSCENARIO_P_H

#include <dspxproject/ClipboardEditScenario.h>

namespace DspxProject {

    class ClipboardEditScenarioPrivate {
        Q_DECLARE_PUBLIC(ClipboardEditScenario)
    public:
        ClipboardEditScenario *q_ptr;
        Document *const document;
        ClipboardEditStrategy *const strategy;
        Clipboard *const clipboard;
        ParameterResamplingScenario *const resampling;
        bool pasteAvailable = false;

        void updatePasteAvailable();
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSCENARIO_P_H
