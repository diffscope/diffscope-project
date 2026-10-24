// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_PACKAGEMANAGERADDON_H
#define DIFFSCOPE_SYNTH_INTERNAL_PACKAGEMANAGERADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Synth::Internal {

    class PackageManagerController;

    class PackageManagerAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")

    public:
        explicit PackageManagerAddOn(QObject *parent = nullptr);
        ~PackageManagerAddOn() override;

        Q_INVOKABLE void openPackageManager();

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        PackageManagerController *m_packageManager;
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_PACKAGEMANAGERADDON_H
