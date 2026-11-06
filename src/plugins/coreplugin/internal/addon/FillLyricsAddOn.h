// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_FILLLYRICSADDON_H
#define DIFFSCOPE_COREPLUGIN_FILLLYRICSADDON_H

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class FillLyricsAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit FillLyricsAddOn(QObject *parent = nullptr);
        ~FillLyricsAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

    };

}

#endif // DIFFSCOPE_COREPLUGIN_FILLLYRICSADDON_H
