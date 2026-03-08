#include "PlatformWindowModifiedHelper_p.h"

#include <qpa/qplatformwindow.h>

namespace UIShell {

    PlatformWindowModifiedHelper::PlatformWindowModifiedHelper(QObject *parent) : QObject(parent) {
    }

    PlatformWindowModifiedHelper::~PlatformWindowModifiedHelper() = default;

    QWindow *PlatformWindowModifiedHelper::window() const {
        return m_window;
    }

    void PlatformWindowModifiedHelper::setWindow(QWindow *window) {
        if (m_window == window)
            return;

        m_window = window;
        Q_EMIT windowChanged();

        // Update platform window modified state when window changes
        updatePlatformWindowModified();
    }

    bool PlatformWindowModifiedHelper::windowModified() const {
        return m_windowModified;
    }

    void PlatformWindowModifiedHelper::setWindowModified(bool modified) {
        if (m_windowModified == modified)
            return;

        m_windowModified = modified;
        Q_EMIT windowModifiedChanged();

        // Update platform window modified state when modified flag changes
        updatePlatformWindowModified();
    }

    void PlatformWindowModifiedHelper::updatePlatformWindowModified() {
        if (!m_window)
            return;

        QPlatformWindow *platformWindow = m_window->handle();
        if (!platformWindow)
            return;

        platformWindow->setWindowModified(m_windowModified);
    }

}
