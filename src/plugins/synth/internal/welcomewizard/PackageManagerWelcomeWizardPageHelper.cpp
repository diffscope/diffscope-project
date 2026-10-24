// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageManagerWelcomeWizardPageHelper.h"

#include <CoreApi/windowsystem.h>

#include <coreplugin/ActionWindowInterfaceBase.h>
#include <coreplugin/CoreInterface.h>

#include <synth/internal/PackageManagerAddOn.h>

namespace Synth::Internal {

    PackageManagerWelcomeWizardPageHelper::PackageManagerWelcomeWizardPageHelper(QObject *parent) : QObject(parent) {
    }

    void PackageManagerWelcomeWizardPageHelper::openPackageManager(QWindow *parent) {
        auto windowInterface = Core::CoreInterface::windowSystem()->firstWindowOfType<Core::ActionWindowInterfaceBase>();
        if (!windowInterface)
            return;
        auto addOn = windowInterface->getFirstObject<PackageManagerAddOn>();
        if (addOn)
            addOn->openPackageManager(parent);
    }

}

#include "moc_PackageManagerWelcomeWizardPageHelper.cpp"
