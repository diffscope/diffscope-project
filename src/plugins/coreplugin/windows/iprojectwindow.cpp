#include "iprojectwindow.h"

#include <QQmlComponent>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/icore.h>

namespace Core {

    static IProjectWindow *m_instance = nullptr;

    class IProjectWindowPrivate {
        Q_DECLARE_PUBLIC(IProjectWindow)
    public:
        IProjectWindow *q_ptr;
        QAK::QuickActionContext *actionContext;
        void init() {
            Q_Q(IProjectWindow);
            actionContext = new QAK::QuickActionContext(q);
            initActionContext();
            ICore::actionRegistry()->addContext(actionContext);
        }

        void initActionContext() {
            Q_Q(IProjectWindow);
            actionContext->setMenuComponent(new QQmlComponent(ICore::qmlEngine(), "SVSCraft.UIComponents", "Menu", q));
            actionContext->setSeparatorComponent(new QQmlComponent(ICore::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", q));
            actionContext->setStretchComponent(new QQmlComponent(ICore::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", q));
            {
                QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "GlobalActions");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto o = component.create();
                o->setParent(q);
                QMetaObject::invokeMethod(o, "registerToContext", actionContext);
            }
            {
                QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "ProjectActions");
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

    IProjectWindow *IProjectWindow::instance() {
        return m_instance;
    }
    QAK::QuickActionContext *IProjectWindow::actionContext() const {
        Q_D(const IProjectWindow);
        return d->actionContext;
    }
    QWindow *IProjectWindow::createWindow(QObject *parent) const {
        Q_D(const IProjectWindow);
        QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "ProjectWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QWindow *>(component.createWithInitialProperties({
            {"windowHandle", QVariant::fromValue(this)}
        }));
        Q_ASSERT(win);
        return win;
    }
    IProjectWindow::IProjectWindow(QObject *parent) : IProjectWindow(*new IProjectWindowPrivate, parent) {
        m_instance = this;
    }
    IProjectWindow::IProjectWindow(IProjectWindowPrivate &d, QObject *parent) : IWindow(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }
    IProjectWindow::~IProjectWindow() {
        m_instance = nullptr;
    }
    void IProjectWindow::nextLoadingState(State nextState) {
        Q_D(IProjectWindow);
        if (nextState == Initialized) {
            d->actionContext->updateElement(QAK::AE_Layouts);
        }
    }
    IProjectWindowRegistry *IProjectWindowRegistry::instance() {
        static IProjectWindowRegistry reg;
        return &reg;
    }
}

#include "moc_iprojectwindow.cpp"
