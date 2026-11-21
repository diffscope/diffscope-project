// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_VISUALIZER_AUTOPAGESCROLLINGADDON_H
#define DIFFSCOPE_AUDIO_VISUALIZER_AUTOPAGESCROLLINGADDON_H

#include <CoreApi/windowinterface.h>

namespace AudioVisualizer::Internal {

    class AutoPageScrollingAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT

    public:
        explicit AutoPageScrollingAddOn(QObject *parent = nullptr);
        void initialize() override;
        void extensionsInitialized() override;
    };

}

#endif //DIFFSCOPE_AUDIO_VISUALIZER_AUTOPAGESCROLLINGADDON_H
