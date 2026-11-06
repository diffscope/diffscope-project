// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSTRATEGY_H

#include <QObject>

#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class DSPXPROJECT_EXPORT ClipboardEditStrategy : public QObject {
        Q_OBJECT
    public:
        explicit ClipboardEditStrategy(QObject *parent = nullptr);
        ~ClipboardEditStrategy() override;

        virtual void reportPasteFailure(QQuickWindow *window) = 0;
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CLIPBOARDEDITSTRATEGY_H
