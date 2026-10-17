// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERADDON_H
#define DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERADDON_H

#include <CoreApi/windowinterface.h>

namespace Synth::Internal {

    class DsspServerAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit DsspServerAddOn(QObject *parent = nullptr);
        ~DsspServerAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_DSSPSERVERADDON_H
