// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UISHELL_TABFOCUSTREEVIEW_P_H
#define UISHELL_TABFOCUSTREEVIEW_P_H

#include <qqmlintegration.h>

#include <QtQuick/private/qquicktreeview_p.h>

namespace UIShell {

    class TabFocusTreeView : public QQuickTreeView {
        Q_OBJECT
        QML_ELEMENT

    public:
        explicit TabFocusTreeView(QQuickItem *parent = nullptr);

    protected:
        void keyPressEvent(QKeyEvent *event) override;
    };

}

#endif // UISHELL_TABFOCUSTREEVIEW_P_H
