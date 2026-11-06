// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CloseSaveCheckAddOn.h"

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectLifecycleScenario.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    CloseSaveCheckAddOn::CloseSaveCheckAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    CloseSaveCheckAddOn::~CloseSaveCheckAddOn() = default;

    void CloseSaveCheckAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        windowInterface->addCloseCallback([windowInterface] {
            return CoreInterface::projectLifecycleScenario()->prepareClose(windowInterface->projectDocumentContext(), windowInterface->window()) == ProjectLifecycleScenario::Status::Success;
        });
    }

    void CloseSaveCheckAddOn::extensionsInitialized() {
    }

    bool CloseSaveCheckAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}
