// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TabFocusTreeView_p.h"

#include <QKeyEvent>

namespace UIShell {

    TabFocusTreeView::TabFocusTreeView(QQuickItem *parent) : QQuickTreeView(parent) {
    }

    void TabFocusTreeView::keyPressEvent(QKeyEvent *event) {
        if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab) {
            event->ignore();
            return;
        }
        QQuickTreeView::keyPressEvent(event);
    }

}
