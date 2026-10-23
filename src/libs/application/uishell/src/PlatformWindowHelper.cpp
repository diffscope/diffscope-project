// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "PlatformWindowHelper_p.h"

#include <QPlatformSurfaceEvent>

#include <qpa/qplatformwindow.h>

namespace UIShell {

    PlatformWindowHelper::PlatformWindowHelper(QObject *parent) : QObject(parent) {
    }

    PlatformWindowHelper::~PlatformWindowHelper() = default;

    QWindow *PlatformWindowHelper::window() const {
        return m_window;
    }

    void PlatformWindowHelper::setWindow(QWindow *window) {
        if (m_window == window)
            return;

        if (m_window) {
            m_window->removeEventFilter(this);
            disconnect(m_window, nullptr, this, nullptr);
        }

        m_window = window;
        if (m_window) {
            m_window->installEventFilter(this);
            connect(m_window, &QWindow::windowTitleChanged, this, &PlatformWindowHelper::windowTitleChanged);
            connect(m_window, &QObject::destroyed, this, [this] {
                setWindow(nullptr);
            });
        }

        updatePlatformWindow();
        Q_EMIT windowChanged();
        Q_EMIT windowTitleChanged();
    }

    bool PlatformWindowHelper::windowModified() const {
        return m_windowModified;
    }

    void PlatformWindowHelper::setWindowModified(bool modified) {
        if (m_windowModified == modified)
            return;

        m_windowModified = modified;
        Q_EMIT windowModifiedChanged();

        updatePlatformWindow();
    }

    QString PlatformWindowHelper::windowTitle() const {
        if (!m_window)
            return {};

        if (const auto platformWindow = m_window->handle())
            return platformWindow->windowTitle();

        return m_window->title();
    }

    bool PlatformWindowHelper::eventFilter(QObject *watched, QEvent *event) {
        if (watched == m_window && event->type() == QEvent::PlatformSurface) {
            const auto surfaceEvent = static_cast<QPlatformSurfaceEvent *>(event);
            if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
                updatePlatformWindow();
                Q_EMIT windowTitleChanged();
            }
        }
        return QObject::eventFilter(watched, event);
    }

    void PlatformWindowHelper::updatePlatformWindow() {
        if (!m_window)
            return;

        QPlatformWindow *platformWindow = m_window->handle();
        if (!platformWindow)
            return;

        // Some platform plugins cache the title only in setWindowTitle().
        platformWindow->setWindowTitle(m_window->title());
        platformWindow->setWindowModified(m_windowModified);
    }

}
