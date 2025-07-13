#include "ihomewindow.h"

#include <QQmlComponent>

#include <coreplugin/icore.h>

namespace Core {

    static IHomeWindow *m_instance = nullptr;

    IHomeWindow *IHomeWindow::instance() {
        return m_instance;
    }
    QWindow *IHomeWindow::createWindow(QObject *parent) const {
        QQmlComponent component(ICore::qmlEngine(), ":/qt/qml/DiffScope/CorePlugin/qml/HomeWindow.qml");
        auto win = component.create();
        return static_cast<QWindow *>(win);
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