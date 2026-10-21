// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_FILEDROPADDON_H
#define DIFFSCOPE_COREPLUGIN_FILEDROPADDON_H

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class FileDropAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit FileDropAddOn(QObject *parent = nullptr);
        ~FileDropAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_FILEDROPADDON_H
