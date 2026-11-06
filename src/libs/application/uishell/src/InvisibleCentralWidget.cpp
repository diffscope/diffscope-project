// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "InvisibleCentralWidget_p.h"

#include <QWindow>

namespace UIShell {
    InvisibleCentralWidget::InvisibleCentralWidget(QWidget *parent) : QWidget(parent) {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput);
        setAttribute(Qt::WA_TranslucentBackground);
        createWinId();
    }
    InvisibleCentralWidget::~InvisibleCentralWidget() {
        void(0);
    }
}
