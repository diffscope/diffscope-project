// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DialogPositioner.h"

#include <QCursor>
#include <QQuickWindow>

namespace UIShell {

    QPointF DialogPositioner::position(const QSizeF &area, const QSizeF &popup, const QVariant &topMargin) {
        return {(area.width() - popup.width()) / 2, topMargin.isValid() ? topMargin.toDouble() : (area.height() - popup.height()) / 2};
    }

    QPointF DialogPositioner::positionAt(const QSizeF &area, const QSizeF &popup, const QPointF &anchor) {
        return {
            qBound(qreal(0), anchor.x(), qMax(qreal(0), area.width() - popup.width())),
            qBound(qreal(0), anchor.y(), qMax(qreal(0), area.height() - popup.height())),
        };
    }

    void DialogPositioner::positionPopup(QObject *popup, QQuickWindow *window, Placement placement) {
        const QSizeF area(window->size());
        const QSizeF size(popup->property("width").toDouble(), popup->property("height").toDouble());
        const auto origin = placement == Placement::AtCursor
            ? positionAt(area, size, window->mapFromGlobal(QCursor::pos()))
            : position(area, size, window->property("popupTopMarginHint"));
        popup->setProperty("x", origin.x());
        popup->setProperty("y", origin.y());
    }

}
