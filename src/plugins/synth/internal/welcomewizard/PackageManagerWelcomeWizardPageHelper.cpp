// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageManagerWelcomeWizardPageHelper.h"

#include <synth/internal/PackageManagerController.h>

namespace Synth::Internal {

    PackageManagerWelcomeWizardPageHelper::PackageManagerWelcomeWizardPageHelper(QObject *parent) : QObject(parent), m_packageManager(new PackageManagerController(this)) {
    }

    void PackageManagerWelcomeWizardPageHelper::openPackageManager(QWindow *parent) {
        m_packageManager->openPackageManager(parent);
    }

}

#include "moc_PackageManagerWelcomeWizardPageHelper.cpp"
