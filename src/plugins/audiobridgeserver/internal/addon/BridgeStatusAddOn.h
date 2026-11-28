// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOBRIDGESERVER_BRIDGESTATUSADDON_H
#define DIFFSCOPE_AUDIOBRIDGESERVER_BRIDGESTATUSADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace AudioBridge::Internal {

    class BridgeStatusAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit BridgeStatusAddOn(QObject *parent = nullptr);
        ~BridgeStatusAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
    };

}

#endif // DIFFSCOPE_AUDIOBRIDGESERVER_BRIDGESTATUSADDON_H
