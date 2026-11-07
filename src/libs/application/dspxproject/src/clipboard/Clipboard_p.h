// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CLIPBOARD_P_H
#define DIFFSCOPE_DSPXPROJECT_CLIPBOARD_P_H

#include <dspxproject/Clipboard.h>

#include <QHash>
#include <QtGlobal>

#include <dspxproject/ClipboardData.h>

namespace DspxProject {

    class Clipboard;

    class ClipboardPrivate {
        Q_DECLARE_PUBLIC(Clipboard)
    public:
        void handleClipboardChanged();

        Clipboard *q_ptr = nullptr;
        Clipboard::Mode mode = Clipboard::Global;
        QHash<ClipboardData::Type, ClipboardData> internalData;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CLIPBOARD_P_H
