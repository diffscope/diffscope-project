// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_INTLEXTENSION_H
#define DIFFSCOPE_BATCHPROCESS_INTLEXTENSION_H

#include <QObject>

namespace BatchProcess {
    class BatchProcessInterface;
}

namespace BatchProcess::Internal {

    class IntlExtension : public QObject {
        Q_OBJECT
    public:
        explicit IntlExtension(BatchProcessInterface *batchProcessInterface, QObject *parent = nullptr);
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_INTLEXTENSION_H
