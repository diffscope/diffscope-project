// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_DEFAULTLYRICMANAGER_P_H
#define DIFFSCOPE_SINGERCATALOG_DEFAULTLYRICMANAGER_P_H

#include <singercatalog/DefaultLyricManager.h>

namespace SingerCatalog {

    class DefaultLyricManagerPrivate {
        Q_DECLARE_PUBLIC(DefaultLyricManager)
    public:
        DefaultLyricManager *q_ptr{};
        const SingerRegistry *const singerRegistry;
        QString fallback;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_DEFAULTLYRICMANAGER_P_H
