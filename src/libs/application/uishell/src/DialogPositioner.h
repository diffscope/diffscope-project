// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_UISHELL_DIALOGPOSITIONER_H
#define DIFFSCOPE_UISHELL_DIALOGPOSITIONER_H

#include <QObject>
#include <QPointF>
#include <QSizeF>
#include <QVariant>
#include <qqmlintegration.h>

#include <uishell/UIShellGlobal.h>

class QQuickWindow;

namespace UIShell {

    class UISHELL_EXPORT DialogPositioner : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
    public:
        using QObject::QObject;

        enum class Placement { WindowDefault, AtCursor };

        Q_INVOKABLE static QPointF position(const QSizeF &area, const QSizeF &popup, const QVariant &topMargin = QVariant());
        Q_INVOKABLE static QPointF positionAt(const QSizeF &area, const QSizeF &popup, const QPointF &anchor);
        static void positionPopup(QObject *popup, QQuickWindow *window, Placement placement = Placement::WindowDefault);
    };

}

#endif // DIFFSCOPE_UISHELL_DIALOGPOSITIONER_H
