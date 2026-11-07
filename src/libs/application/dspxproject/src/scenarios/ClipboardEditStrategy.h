// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSTRATEGY_H

#include <QObject>

#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    /** Supplies clipboard editing diagnostics. Window arguments are optional dialog parents. */
    class DSPXPROJECT_EXPORT ClipboardEditStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit ClipboardEditStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~ClipboardEditStrategy() override;

        /** Reports unavailable or invalid clipboard data. */
        virtual void reportPasteFailure(QQuickWindow *window) = 0;
        /** Reports a conflicting operation that prevents a transaction. */
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSTRATEGY_H
