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
