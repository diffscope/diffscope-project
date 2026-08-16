// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_HASHHELPER_H
#define DIFFSCOPE_AUDIO_HASHHELPER_H

#include <QString>

namespace Audio::Internal {

    class HashHelper {
    public:
        static QString digest(const QString &filePath);
    };

}

#endif // DIFFSCOPE_AUDIO_HASHHELPER_H
