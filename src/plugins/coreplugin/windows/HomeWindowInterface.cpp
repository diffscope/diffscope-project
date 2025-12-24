#include "HomeWindowInterface.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/ActionHelper.h>

namespace Core {

    static HomeWindowInterface *m_instance = nullptr;

    class HomeWindowInterfacePrivate {
        Q_DECLARE_PUBLIC(HomeWindowInterface)
    public:
        HomeWindowInterface *q_ptr;
        void init() {
            Q_Q(HomeWindowInterface);
            initActionContext();
        }
        void initActionContext() {
            Q_Q(HomeWindowInterface);
            auto actionContext = q->actionContext();
            {
                QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "GlobalActions");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto o = component.createWithInitialProperties({
                    {"windowHandle", QVariant::fromValue(q)},
                });
                o->setParent(q);
                QMetaObject::invokeMethod(o, "registerToContext", actionContext);
            }
            {
                QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "HomeActions");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto o = component.createWithInitialProperties({
                    {"windowHandle", QVariant::fromValue(q)},
                });
                o->setParent(q);
                QMetaObject::invokeMethod(o, "registerToContext", actionContext);
            }
        }
    };

    HomeWindowInterface *HomeWindowInterface::instance() {
        return m_instance;
    }
    QWindow *HomeWindowInterface::createWindow(QObject *parent) const {
        Q_D(const HomeWindowInterface);
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "HomeWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QWindow *>(component.createWithInitialProperties({{"windowHandle", QVariant::fromValue(this)}}));
        Q_ASSERT(win);
        return win;
    }
    HomeWindowInterface::HomeWindowInterface(QObject *parent) : HomeWindowInterface(*new HomeWindowInterfacePrivate, parent) {
        m_instance = this;
    }
    HomeWindowInterface::HomeWindowInterface(HomeWindowInterfacePrivate &d, QObject *parent) : ActionWindowInterfaceBase(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }
    HomeWindowInterface::~HomeWindowInterface() {
        m_instance = nullptr;
    }
    HomeWindowInterfaceRegistry *HomeWindowInterfaceRegistry::instance() {
        static HomeWindowInterfaceRegistry reg;
        return &reg;
    }
}

#include "moc_HomeWindowInterface.cpp"
