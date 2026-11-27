// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CloseSaveCheckAddOn.h"

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <dspxproject/ProjectLifecycleScenario.h>

namespace Core::Internal {

    CloseSaveCheckAddOn::CloseSaveCheckAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    CloseSaveCheckAddOn::~CloseSaveCheckAddOn() = default;

    void CloseSaveCheckAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        windowInterface->addCloseCallback([windowInterface] {
            if (windowInterface->isManaged()) {
                if (RuntimeInterface::exitInProgress()) {
                    windowInterface->projectDocumentContext()->preserveRecoveryLog();
                    return true;
                }
                windowInterface->window()->hide();
                return false;
            }
            return CoreInterface::projectLifecycleScenario()->prepareClose(windowInterface->projectDocumentContext(), windowInterface->window()) == DspxProject::ProjectLifecycleScenario::Status::Success;
        });
    }

    void CloseSaveCheckAddOn::extensionsInitialized() {
    }

    bool CloseSaveCheckAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}
