// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOBRIDGESERVER_DOWNLOADBRIDGEADDON_H
#define DIFFSCOPE_AUDIOBRIDGESERVER_DOWNLOADBRIDGEADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace AudioBridge::Internal {

    class DownloadBridgeAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit DownloadBridgeAddOn(QObject *parent = nullptr);
        ~DownloadBridgeAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

        Q_INVOKABLE static void openDownloadPage();
    };

}

#endif // DIFFSCOPE_AUDIOBRIDGESERVER_DOWNLOADBRIDGEADDON_H
