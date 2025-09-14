#include "ihomewindow.h"

#include <QQmlComponent>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/internal/actionhelper.h>

namespace Core {

    static IHomeWindow *m_instance = nullptr;

    class IHomeWindowPrivate {
        Q_DECLARE_PUBLIC(IHomeWindow)
    public:
        IHomeWindow *q_ptr;
        void init() {
            Q_Q(IHomeWindow);
            initActionContext();
        }
        void initActionContext() {
            Q_Q(IHomeWindow);
            auto actionContext = q->actionContext();
            QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.Core", "HomeActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"windowHandle", QVariant::fromValue(q)},
            });
            o->setParent(q);
            QMetaObject::invokeMethod(o, "registerToContext", actionContext);
        }
    };

    IHomeWindow *IHomeWindow::instance() {
        return m_instance;
    }
    QWindow *IHomeWindow::createWindow(QObject *parent) const {
        Q_D(const IHomeWindow);
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.Core", "HomeWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QWindow *>(component.createWithInitialProperties({{"windowHandle", QVariant::fromValue(this)}}));
        Q_ASSERT(win);
        return win;
    }
    IHomeWindow::IHomeWindow(QObject *parent) : IHomeWindow(*new IHomeWindowPrivate, parent) {
        m_instance = this;
    }
    IHomeWindow::IHomeWindow(IHomeWindowPrivate &d, QObject *parent) : IActionWindowBase(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }
    IHomeWindow::~IHomeWindow() {
        m_instance = nullptr;
    }
    IHomeWindowRegistry *IHomeWindowRegistry::instance() {
        static IHomeWindowRegistry reg;
        return &reg;
    }
}

#include "moc_ihomewindow.cpp"
