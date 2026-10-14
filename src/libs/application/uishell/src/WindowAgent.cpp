// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WindowAgent_p.h"

#include <QQuickWindow>
#include <QRect>

namespace UIShell {

    WindowAgent::WindowAgent(QObject *parent) : QWK::QuickWindowAgent(parent) {
    }

    WindowAgent::~WindowAgent() = default;

    bool WindowAgent::setup(QQuickWindow *window) {
        if (!window)
            return false;

#ifdef Q_OS_WINDOWS
        const auto geometry = window->geometry();
#endif

        if (!QWK::QuickWindowAgent::setup(window))
            return false;

#ifdef Q_OS_WINDOWS
        // Removing the system title bar expands the client area while preserving the outer frame.
        window->create();
        if (window->windowState() == Qt::WindowNoState)
            window->setGeometry(geometry);
#endif

        return true;
    }

}
