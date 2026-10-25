// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#include "ASIOPlugin.h"

#include <TalcsDevice/AudioDriverManager.h>
#include <TalcsAsio/ASIOAudioDriver.h>

#include <audio/GlobalAudioContext.h>

namespace ASIO::Internal {

    bool ASIOPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        auto driverManager = Audio::GlobalAudioContext::driverManager();
        driverManager->addAudioDriver(new talcs::ASIOAudioDriver);
        return true;
    }

    void ASIOPlugin::extensionsInitialized() {
    }

}
