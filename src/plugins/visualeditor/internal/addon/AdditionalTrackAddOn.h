// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUAL_EDITOR_ADDITIONALTRACKADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_ADDITIONALTRACKADDON_H

#include <CoreApi/windowinterface.h>

class QQmlComponent;

namespace VisualEditor::Internal {

    class AdditionalTrackAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(QQmlComponent *phonemePanelBackgroundComponent READ phonemePanelBackgroundComponent CONSTANT)

    public:
        explicit AdditionalTrackAddOn(QObject *parent = nullptr);
        ~AdditionalTrackAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static QQmlComponent *phonemePanelBackgroundComponent();
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_ADDITIONALTRACKADDON_H
