// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "IFileOpenHandler.h"
#include "IFileOpenHandler_p.h"

namespace Core {

    IFileOpenHandler::IFileOpenHandler(QObject *parent) : QObject(parent), d_ptr(new IFileOpenHandlerPrivate) {
        Q_D(IFileOpenHandler);
        d->q_ptr = this;
    }

    IFileOpenHandler::~IFileOpenHandler() = default;

    QSet<qsizetype> IFileOpenHandler::preflight(const QStringList &, QWindow *) const {
        return {};
    }

}

#include "moc_IFileOpenHandler.cpp"
