// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_IFILEOPENHANDLER_P_H
#define DIFFSCOPE_COREPLUGIN_IFILEOPENHANDLER_P_H

#include <coreplugin/IFileOpenHandler.h>

namespace Core {

    class IFileOpenHandlerPrivate {
        Q_DECLARE_PUBLIC(IFileOpenHandler)
    public:
        IFileOpenHandler *q_ptr = nullptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_IFILEOPENHANDLER_P_H
