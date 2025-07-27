#include "ihomewindow.h"

#include <QQmlComponent>

#include <coreplugin/icore.h>

namespace Core {

    static IHomeWindow *m_instance = nullptr;

    IHomeWindow *IHomeWindow::instance() {
        return m_instance;
    }
    QWindow *IHomeWindow::createWindow(QObject *parent) const {
        QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "HomeWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QWindow *>(component.create());
        Q_ASSERT(win);
        return win;
    }
    IHomeWindow::IHomeWindow(QObject *parent) : IWindow(parent) {
        m_instance = this;
    }
    IHomeWindow::~IHomeWindow() {
        m_instance = nullptr;
    }
    IHomeWindowRegistry *IHomeWindowRegistry::instance() {
        static IHomeWindowRegistry reg;
        return &reg;
    }
}